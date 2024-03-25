package firesim.configs

import org.chipsalliance.cde.config.Config
import midas._

class RocketTilePCISF1Config extends Config(
  new WithFireAxePCISConfig("RocketTile") ++
  new BaseF1Config)

class DualRocketTilePCISF1Config extends Config(
  new WithFireAxePCISConfig("RocketTile.0~1") ++
  new BaseF1Config)

class QuadRocketTilePCISF1Config extends Config(
  new WithFireAxePCISConfig("RocketTile.0~3") ++
  new BaseF1Config)

class RocketTileQSFPXilinxAlveoConfig extends Config(
  new WithFireAxeQSFPConfig("RocketTile") ++
  new BaseXilinxAlveoConfig
)

class HyperscaleAccelsQSFPXilinxAlveoConfig extends Config(
  new WithFireAxeQSFPConfig("SnappyCompressor+SnappyDecompressor+ProtoAccel+ProtoAccelSerializer") ++
  new BaseXilinxAlveoConfig)

class QuadTileRingNoCTopoQSFPXilinxAlveoConfig extends Config(
  new WithFireAxeQSFPNoCConfig("0~1+2~3+4~9") ++
  new BaseXilinxAlveoConfig)

class OctaTileMeshNoCTopoQSFPXilinxAlveoConfig extends Config(
  new WithFireAxeQSFPNoCConfig("0.1.4.5+2.3.6.7+8~15") ++
  new BaseXilinxAlveoConfig)

class DualRocketTileQSFPXilinxAlveoConfig extends Config(
  new WithFireAxeQSFPConfig("RocketTile.0~1") ++
  new BaseXilinxAlveoConfig)

class Sha3QSFPXilinxAlveoConfig extends Config(
  new WithFireAxeQSFPConfig("Sha3Accel") ++
  new BaseXilinxAlveoConfig)

class GemminiQSFPXilinxAlveoConfig extends Config(
  new WithFireAxeQSFPConfig("Gemmini") ++
  new BaseXilinxAlveoConfig)

class BoomQSFPXilinxAlveoConfig extends Config(
  new WithFireAxeQSFPConfig("BoomBackend") ++
  new BaseXilinxAlveoConfig)
