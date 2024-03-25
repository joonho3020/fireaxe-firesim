// See LICENSE for license details.

package midas
package passes

import midas.stage._
import midas.passes.partition._


import firrtl._


class SplitModulesByPortTransforms extends Transform {
  def inputForm = LowForm
  def outputForm = LowForm

  def execute(state: CircuitState) = {
    println(s"Starting SplitModules Transforms")
    val xforms = Seq(
      new ResolveAndCheck,
      HoistStopAndPrintfEnables,
      firrtl.passes.RemoveValidIf,
      new firrtl.transforms.ConstantPropagation,
      firrtl.passes.SplitExpressions,
      firrtl.passes.ResolveKinds,
      firrtl.passes.CommonSubexpressionElimination,
      new firrtl.transforms.DeadCodeElimination,
      new firrtl.transforms.InferResets,
      firrtl.passes.CheckTypes,
      new HighFirrtlToMiddleFirrtl,
      new MiddleFirrtlToLowFirrtl,
      new LowerStatePass,
      new EmitFirrtl("pre-processed.fir"),
      new SplitModulesByPortsStandalone,
      new EmitFirrtl("post-split.fir"),
      new ResolveAndCheck
      )
    (xforms foldLeft state)((in, xform) =>
    xform runTransform in).copy(form=outputForm)
  }
}
