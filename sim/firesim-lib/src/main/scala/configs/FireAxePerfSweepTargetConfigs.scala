package firesim.configs

import org.chipsalliance.cde.config.Config
import midas._

/////////////////////////////////////////////////////////////////////////////

class OneRocketTileQSFPXilinxAlveoConfig extends Config(
  new WithFireAxeQSFPConfig("RocketTile") ++              // Split RocketTile
  new BaseXilinxAlveoConfig)

class TwoRocketTileQSFPXilinxAlveoConfig extends Config(
  new WithFireAxeQSFPConfig("RocketTile.0~1") ++          // Split 2 RocketTiles
  new BaseXilinxAlveoConfig)

class FourRocketTileQSFPXilinxAlveoConfig extends Config(
  new WithFireAxeQSFPConfig("RocketTile.0~3") ++          // Split 4 RocketTiles
  new BaseXilinxAlveoConfig)

class EightRocketTileQSFPXilinxAlveoConfig extends Config(
  new WithFireAxeQSFPConfig("RocketTile.0~7") ++          // Split 8 RocketTiles
  new BaseXilinxAlveoConfig)


class SixteenRocketTileQSFPXilinxAlveoConfig extends Config(
  new WithFireAxeQSFPConfig("RocketTile.0~15") ++          // Split 16 RocketTiles
  new BaseXilinxAlveoConfig)

/////////////////////////////////////////////////////////////////////////////

class OneBoomTileQSFPXilinxAlveoConfig extends Config(
  new WithFireAxeQSFPConfig("BoomTile") ++              // Split RocketTile
  new BaseXilinxAlveoConfig)

class TwoBoomTileQSFPXilinxAlveoConfig extends Config(
  new WithFireAxeQSFPConfig("BoomTile.0~1") ++          // Split 2 BoomTiles
  new BaseXilinxAlveoConfig)

class ThreeBoomTileQSFPXilinxAlveoConfig extends Config(
  new WithFireAxeQSFPConfig("BoomTile.0~2") ++          // Split 3 BoomTiles
  new BaseXilinxAlveoConfig)

class FourBoomTileQSFPXilinxAlveoConfig extends Config(
  new WithFireAxeQSFPConfig("BoomTile.0~3") ++          // Split 4 BoomTiles
  new BaseXilinxAlveoConfig)

class FiveBoomTileQSFPXilinxAlveoConfig extends Config(
  new WithFireAxeQSFPConfig("BoomTile.0~4") ++          // Split 5 BoomTiles
  new BaseXilinxAlveoConfig)

class SixBoomTileQSFPXilinxAlveoConfig extends Config(
  new WithFireAxeQSFPConfig("BoomTile.0~5") ++          // Split 6 BoomTiles
  new BaseXilinxAlveoConfig)

/////////////////////////////////////////////////////////////////////////////

class Split3FPGARingXilinxAlveoConfig extends Config(
  new WithFireAxeQSFPNoCConfig("0~1+2~4+5~10") ++
  new BaseXilinxAlveoConfig)

class Split4FPGARingXilinxAlveoConfig extends Config(
  new WithFireAxeQSFPNoCConfig("0~1+2~3+4+5~10") ++
  new BaseXilinxAlveoConfig)

class Split5FPGARingXilinxAlveoConfig extends Config(
  new WithFireAxeQSFPNoCConfig("0~1+2+3+4+5~10") ++
  new BaseXilinxAlveoConfig)

class Split6FPGARingXilinxAlveoConfig extends Config(
  new WithFireAxeQSFPNoCConfig("0+1+2+3+4+5~10") ++
  new BaseXilinxAlveoConfig)
