//See LICENSE for license details
package firesim.bridges

import chisel3._
import chisel3.util._
import org.chipsalliance.cde.config.Parameters
import midas.widgets._
import testchipip.TileGenericTraceIO

object GenericTraceConsts {
  val TOKEN_QUEUE_DEPTH = 6144
}

case class GenericTraceKey(bitWidth: Int)

class GenericTraceTargetIO(val bitWidth : Int) extends Bundle {
// val tracevTrigger = Input(Bool())
  val trace = Input(new TileGenericTraceIO(bitWidth))
}

class GenericTraceBridge(val bitWidth: Int) extends BlackBox
    with Bridge[HostPortIO[GenericTraceTargetIO], GenericTraceBridgeModule] {
  val io = IO(new GenericTraceTargetIO(bitWidth))
  val bridgeIO = HostPort(io)
  val constructorArg = Some(GenericTraceKey(bitWidth))
  generateAnnotations()
}

object GenericTraceBridge {
  def apply(generic_trace: TileGenericTraceIO)(implicit p:Parameters): GenericTraceBridge = {
    val ep = Module(new GenericTraceBridge(generic_trace.bitWidth))
    ep.io.trace := generic_trace
    ep
  }
}

class GenericTraceBridgeModule(key: GenericTraceKey)(implicit p: Parameters)
  extends BridgeModule[HostPortIO[GenericTraceTargetIO]]()(p)
  with StreamToHostCPU {

  val toHostCPUQueueDepth = GenericTraceConsts.TOKEN_QUEUE_DEPTH

  lazy val module = new BridgeModuleImp(this) {
    val io = IO(new WidgetIO)
    val hPort = IO(HostPort(new GenericTraceTargetIO(key.bitWidth)))

    // Set after trigger-dependent memory-mapped registers have been set, to
    // prevent spurious credits
    val initDone    = genWORegInit(Wire(Bool()), "initDone", false.B)
    val traceEnable    = genWORegInit(Wire(Bool()), "traceEnable", false.B)

    // Trigger Selector
    val triggerSelector = RegInit(0.U((p(CtrlNastiKey).dataBits).W))
    attach(triggerSelector, "triggerSelector", WriteOnly)

    // Mask off ready samples when under reset
    val trace = hPort.hBits.trace.data
    val traceValid = trace.valid && !hPort.hBits.trace.reset.asBool()
// val triggerTraceV = hPort.hBits.tracevTrigger

    // Connect trigger
// val trigger = MuxLookup(triggerSelector, false.B, Seq(
// 0.U -> true.B,
// 1.U -> triggerTraceV
// ))

// triggerTraceV := false.B

    val trigger = (triggerSelector === 0.U)
    val traceOut = initDone && traceEnable && traceValid && trigger

    // Width of the trace vector
    val traceWidth = trace.bits.getWidth
    // Width of one token as defined by the DMA
    val discreteDmaWidth = toHostStreamWidthBits
    // How many tokens we need to trace out the bit vector, at least one for DMA sanity
    val tokensPerTrace = math.max((traceWidth + discreteDmaWidth - 1) / discreteDmaWidth, 1)

    println( "GenericTraceBridgeModule")
    println(s"    traceWidth      ${traceWidth}")
    println(s"    dmaTokenWidth   ${discreteDmaWidth}")
    println(s"    requiredTokens  {")
    for (i <- 0 until tokensPerTrace)  {
      val from = ((i + 1) * discreteDmaWidth) - 1
      val to   = i * discreteDmaWidth
      println(s"        ${i} -> traceBits(${from}, ${to})")
    }
    println( "    }")
    println( "")

    // TODO: the commented out changes below show how multi-token transfers would work
    // However they show a bad performance for yet unknown reasons in terms of FPGA synth
    // timings -- verilator shows expected results
    // for now we limit us to 512 bits with an assert.
    if (tokensPerTrace > 1) {
      // State machine that controls which token we are sending and whether we are finished
      val tokenCounter = new Counter(tokensPerTrace)
      val readyNextTrace = WireInit(true.B)
      when (streamEnq.fire) {
        readyNextTrace := tokenCounter.inc()
      }

      val paddedTrace = trace.bits.asUInt().pad(tokensPerTrace * discreteDmaWidth)
      val paddedTraceSeq = for (i <- 0 until tokensPerTrace) yield {
        i.U -> paddedTrace(((i + 1) * discreteDmaWidth) - 1, i * discreteDmaWidth)
      }

      streamEnq.valid := hPort.toHost.hValid && traceOut
      streamEnq.bits := MuxLookup(tokenCounter.value , 0.U, paddedTraceSeq)
      hPort.toHost.hReady := initDone && streamEnq.ready && readyNextTrace
    } else {
      streamEnq.valid := hPort.toHost.hValid && traceOut
      streamEnq.bits := trace.bits.asUInt().pad(discreteDmaWidth)
      hPort.toHost.hReady := initDone && streamEnq.ready
    }

    hPort.fromHost.hValid := true.B

    genCRFile()

    override def genHeader(base: BigInt, memoryRegions: Map[String, BigInt], sb: StringBuilder): Unit = {
      genConstructor(
          base,
          sb,
          "generictrace_t",
          "generictrace",
          Seq(
            UInt32(toHostStreamIdx),
            UInt32(toHostCPUQueueDepth),
            UInt32(discreteDmaWidth),
            UInt32(traceWidth),
            Verbatim(clockDomainInfo.toC)
            ),
          hasStreams = true
      )
    }
  }
}
