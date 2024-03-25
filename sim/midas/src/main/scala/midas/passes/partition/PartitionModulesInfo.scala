package midas.passes.partition

import scala.collection.mutable
import Array.range

import firrtl._
import firrtl.ir._
import firrtl.Mappers._
import firrtl.annotations._
import firrtl.analyses.{InstanceKeyGraph, InstanceGraph}

import midas._
import midas.stage._
import midas.targetutils._

import org.chipsalliance.cde.config.{Parameters, Config}

object PartitionModulesInfo {
  val wrapperPfx = "PartitionWrapper"
  val groupPfx = "Grouped"
  val groupWrapperPfx = "GroupWrapper"

  val fireSimWrapper = "FireSimPartition"
  val extractModuleInstanceName = "extractModuleInstance"

  def getConfigParams(annos: AnnotationSeq): Parameters = {
    annos.collectFirst({
      case midas.stage.phases.ConfigParametersAnnotation(p)  => p
    }).get
  }

  var numGroups: Int = 0

  private def getNumGroups(state: CircuitState): Int = {
    state.circuit.modules.filter { dm =>
      dm.name contains groupPfx
    }.size
  }

  def getGroupName(i: Int): String = {
    s"${groupPfx}_${i}"
  }

  def getGroups(state: CircuitState): (Seq[String], Seq[String]) = {
    if (numGroups == 0) {
      numGroups = getNumGroups(state)
    }
    val groups = range(0, numGroups).map { getGroupName(_) }
    val groupWrappers = groups.map { g => groupWrapperPfx + "_" + g }
    (groups, groupWrappers)
  }
}
