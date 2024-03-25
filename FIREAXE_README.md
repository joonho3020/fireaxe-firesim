# Welcome to the README of FireAxe v0.0.3


# PLEASE DO NOT DISTRIBUTE OR MENTION THIS IN PUBLIC AS WE PLAN TO SUBMIT THIS WORK SOON


## Introduction
### What is this document?
This document serves as a guideline on using FireAxe on EC2-F1. The documentation can also be used to 
run metasims for local debugging purposes as well.


### What is FireAxe?
FireAxe is a extension to FireSim that enables automatic partitioning of the SoC and maps it onto multiple FPGAs, enabling
users to simulate larger designs that do not fit on a single FPGA.

### How FireAxe partitions designs
FireAxe has two main FIRRTL passes called the "ExtractModule" pass & the "RemoveModule" pass. The ExtractModule pass runs to extract the user specified module(s) outside of the SoC. It first wraps the module in a wrapper, analyzes the module interface to automatically insert skid-buffers and replaces certain `valid` signals to `fire` (`valid && ready`) to model backpressure between LI-BDNs. More information about skid-buffers [Skid-Buffer](http://fpgacpu.ca/fpga/Pipeline_Skid_Buffer.html#:~:text=A%20skid%20buffer%20is%20the,smooth%2Dout%20data%20rate%20mismatches.) and LI-BDNs [The LI-BDN Paper](https://dspace.mit.edu/bitstream/handle/1721.1/58834/Vijayaraghavan-2009-Bounded%20Dataflow%20Networks%20and%20Latency-Insensitive%20Circuits.pdf?sequence=1&isAllowed=y) are available in the links provided. After all that is done, it attaches the correct FireSim-bridges to the extracted module so that it can be passed on to our good old Golden-Gate compiler. Specifically, the `CutBoundaryBridge` receives input tokens from the host and slices the token bits according to the input ports of the module so that all the input ports have the correct values. Similarily, it concatentates all the output wires of the module to generate a output token that will be sent to the host.

The RemoveModule pass runs to remove the module(s) that the user specified from the SoC. Similary to the ExtractModule pass, it wraps the module in a wrapper and performs the skid-buffer insertion pass and the valid to fire pass. Then it replaces all the FIRRTL statements inside the module to a `CutBoundaryBridge`. Then the module is passed on to the Golden-Gate compiler which will pull the `CutBoundaryBridge` to the top of the module hierarchy so that it can perform the FAME transformations.


### How each partition communicates to each other
Currently each partitions sends and receives tokens from the other side through the host. For each edge in the partition, the user has to create a `FireSimPipeNode` which is basically a thread running on the host. It exchanges tokens between the per-partition simulation driver via shared memory programming.


### Limitations
Currently, FireAxe only supports partitioning out modules with latency insensitive interfaces (such as modules with decoupled-IOs) as it is difficult to model combinational logic by composing LI-BDNs.
Tiles are a great example. Currently, the RocketTile & BoomTile interface consists of a Tilelink master port and a bunch of interrupts which are both latency insensitive.
RoCC accelerators on the other hand, contains a latency-sensitive interface: the `io.busy` signal which is used just like a fence instruction, i.e., it blocks the core from execuing younger memory instructions.
Hence for RoCCs, we have a utility pass that will preemptively assert the fence signal when a RoCC instruction fires. More details in [Preemptive assertion of io.busy in RoCC](#preemptive-assertion-of-io.busy-in-rocc).
Also, to achieve a higher simulation performance, we transform the target that the user provided. That is, we insert a user-configurable amount of latency (ranging between 5 to 32) in between the partitioned boundaries.
We explain in more detail how the user can change this knob in [Changing the latency to insert in the target](#changing-the-latency-inserted-in-the-target)
The below table shows simulation performance according to the boundary-latency for the `fireaxe_split_quad_rockettiles_from_soc_config` when the target design is synthesized at 70 MHZ.

| Cycles | Simulator Performance (kHZ) |
|--------|-----------------------------|
| 4      | 263                         |
| 5      | 331                         |
| 14     | 835                         |
| 28     | 1181                        |

Another limitation is that the group of modules that are pulled out together have to be in the same clock domain (which isn't a huge problem because the modules under interest will be RoCC accelerators or tiles).


### Caveats
- The `root` partition cannot be multithreaded (FIRRTL Dedup pass blows up when running after a `RemoveModule` pass that removes multiple RoCC accels).


## Tutorial

From now on, we will provide a step-by-step tutorial on using FireAxe. This section assumes that the user is already familiar with using FireSim. If not, please make yourself familiar with FireSim by visiting the FireSim docs [FireSim Docs](https://docs.fires.im/en/stable).

### Basic setup
First, we need to setup Chipyard/FireSim as usual.


### Pulling out a RocketTile
In this section, we will pull out a `RocketTile` from the good old `RocketConfig`. Identical to the original FireSim flow, we need to build bitstreams for each of the FPGA. We will specify each part of the partition inside `config_build_recipes.yaml`. As you can see below, we have two build recipes: `firesim_rocket_split_soc` and `firesim_rocket_split_tile`. `firesim_rocket_split_soc` is the original SoC that contains everything except the `RocketTile` that we will pull out. Converserly, `firesim_rocket_split_tile` contains the `RocketTile` that we pulled out.

#### Example `config_build_recipes.yaml`

The build recipes looks identical to the original FireSim recipe except for the one line that contains `TARGET_SPLIT_CONFIG`.
`TARGET_SPLIT_CONFIG` is basically a combination of `SplitMode`, `ModulesToSplitOut`, `FPGACount`, `FPGAIndex` delimited by `-`.
We now explain what each field means in more detail.

`SplitMode` specifies which FIRRL pass to run (i.e., `ExtractModule` pass or the `RemoveModule` pass). `SplitOuter` will run the `RemoveModule` pass which removes the specified module(s) from the design, replacing them with bridges. On the other hand, `SplitInner` will run the `ExtractModule` pass which extracts out the specified module(s) from the design. We call the design partitioned by the `RemoveModule` (or `SplitOuter`) the `root` design (as it contains all the subsystem and peripheries) and the design partitioned by the `ExtractModule` (or `SplitInner`) the `leaf` design.

`ModulesToSplit` specifies which module names we want to pull out. If there are muliple module instances with the specified name, all of them will be pulled out. Also, if you want to specify multiple modules, you just add them all by delimiting them by  a `+` sign. An example is provided in [Pull Out Multiple Modules](pulling-out-multiple-tiles-and-grouping-them-onto-multiple-fpgas).

`FPGACount` is the number of FPGAs to partition the design onto. Easy-peasy.

`FPGAIndex` is the FPGA index to place the current build recipe onto. The indexing is a bit peculiar. If you have specified `N` FPGAs in `FPGACount`, one FPGA is always reserved as a `Base` FPGA for the `root` design. Then the following FPGAs are assigned to each `leaf` design from `0 ~ N-2`. The indexing can be represented as a dictionary like below.
```python
{
 `Base`: `root`,
 `0`: `leaf-0`,
 .
 .
 .
 `N-2`: `leaf-N-2`
}
```

So for `firesim_rocket_split_soc`, `SplitMode` is `SplitOuter` (as we will remove the tile), `ModulesToSplit` is `RocketTile`, `FPGACount` is 2 and `FPGAIndex` is `Base` (as this is the `root` design).

For `firesim_rocket_split_tile`, `SplitMode` is `SplitInner` (as we will extract the tile), `ModulesToSplit` is `RocketTile`, `FPGACount` is 2 and `FPGAIndex` is `0` (as this is the only `leaf` design).

```yaml
# config_build_recipes.yaml

firesim_rocket_split_soc:
  PLATFORM: f1
  TARGET_PROJECT: firesim
  DESIGN: FireSim
  TARGET_CONFIG: WithDefaultFireSimBridges_WithNoTraceFireSimConfigTweaks_chipyard.RocketConfig
  PLATFORM_CONFIG: BaseF1Config
  TARGET_SPLIT_CONFIG: SplitOuter-RocketTile-2-Base
  deploy_quintuplet: null
  platform_config_args:
    fpga_frequency: 90
    build_strategy: TIMING
  post_build_hook: null
  metasim_customruntimeconfig: null
  bit_builder_recipe: bit-builder-recipes/f1.yaml

firesim_rocket_split_tile:
  PLATFORM: f1
  TARGET_PROJECT: firesim
  DESIGN: FireSim
  TARGET_CONFIG: WithDefaultFireSimBridges_WithNoTraceFireSimConfigTweaks_chipyard.RocketConfig
  PLATFORM_CONFIG: BaseF1Config
  TARGET_SPLIT_CONFIG: SplitInner-RocketTile-2-0
  deploy_quintuplet: null
  platform_config_args:
    fpga_frequency: 90
    build_strategy: TIMING
  post_build_hook: null
  metasim_customruntimeconfig: null
  bit_builder_recipe: bit-builder-recipes/f1.yaml
```

#### Example `config_build.yaml`

Now we place `firesim_rocket_split_soc` and `firesim_rocket_split_tile` in `config_build.yaml` under `builds_to_run` as we
always do.
Then run `firesim buildbitstream` as usual. The FireSim manager will launch a build for both of these build recipes.

```yaml
builds_to_run:
    # this section references builds defined in config_build_recipes.yaml
    # if you add a build here, it will be built when you run buildbitstream

    # Unnetworked designs use a three-domain configuration
    # Tiles: 1600 MHz
    #    <Rational Crossing>
    # Uncore: 800 MHz
    #    <Async Crossing>
    # DRAM : 1000 MHz

    - firesim_rocket_split_soc
    - firesim_rocket_split_tile
```

#### Example `config_hwdb.yaml`

This part is easy. Just copy & paste the created AGFIs into your `config_hwdb.yaml`

```yaml
firesim_rocket_split_soc:
    agfi: agfi-<hash generated>
    deploy_quintuplet_override: null
    custom_runtime_config: null

firesim_rocket_split_tile:
    agfi: agfi-<hash generated>
    deploy_quintuplet_override: null
    custom_runtime_config: null
```

#### Example `user_topology.py`
To enable communication between the partitioned designs, we need to instantiate the `FireSimPipeNode` as mentioned above.
A bunch of example topologies are defined inside `deploy/runtools/user_topology.py`, so lets open that up.

As you can see in the `fireaxe_two_node_base_config`, it instantiates the `FireSimPipeNode` as the topology `root` and adds
a `server_edge` which represents a edge between the partitioned design. The vertices of the `server_edge` are `FireSimServerNode`s 
which represents the FPGA simuilations. Each vertex also has metadata that indicates whether the partitioned design mapped to the
FPGA is a `leaf` partition or not. As you can see, the `hwdb_entries[1]` is mapped to the `leaf` partition (which is the `firesim_rocket_split_tile` in our case) in the below example.

```python
def fireaxe_two_node_base_config(self, hwdb_entries) -> None:
   assert len(hwdb_entries) == 2
   self.roots = [FireSimPipeNode()]
   server_edge = []
   if isinstance(self.roots[0], FireSimPipeNode):
       for (hwdb, base) in hwdb_entries.items():
           server_edge.append(FireSimServerNode(hwdb, partitioned=True, base_partition=base))
       self.roots[0].add_downlink_partition(server_edge)

def fireaxe_split_rocket_tile_from_soc_config(self) -> None:
    hwdb_entries = {
        "firesim_rocket_split_soc"  : True, # True indicates taht this is a root partition
        "firesim_rocket_split_tile" : False # False indicates that this is a leaf partition
    }
    self.fireaxe_two_node_base_config(hwdb_entries)
```

#### Example `config_runtime.yaml`

Now we are almost done. All we need to do is set the `config_runtime[target_config][topology]` to our `firesaxe_split_rocket_tile_from_soc_config` defined above and run `firesim infrasetup && firesim runworkload`.

```yaml
# config_runtime.yaml
target_config:
    topology: fireaxe_split_rocket_tile_from_soc_config
    no_net_num_nodes: 2
    link_latency: 3
    switching_latency: 0
    net_bandwidth: 200
    profile_interval: -1

    # This references a section from config_hwdb.yaml for fpga-accelerated simulation
    # or from config_build_recipes.yaml for metasimulation
    # In homogeneous configurations, use this to set the hardware config deployed
    # for all simulators
    default_hw_config: firesim_rocket_quadcore_no_nic_l2_llc4mb_ddr3

    # Advanced: Specify any extra plusargs you would like to provide when
    # booting the simulator (in both FPGA-sim and metasim modes). This is
    # a string, with the contents formatted as if you were passing the plusargs
    # at command line, e.g. "+a=1 +b=2"
    plusarg_passthrough: ""
```

#### Changing the latency inserted in the target

If you look closely into `config_runtime.yaml`, we have a new field called `partitioning`.
All you need to do is change the `config_runtime[partitioning][batch_size]` field to change the number of cycles
inserted between the partition boundaries.

```yaml
# config_runtime.yaml
partitioning:
    batch_size: 28
```


### Pulling out Gemmini

As mentioned above, the `io.busy` in the RoCC interface is not latency insensitive because it is combinationally
tied to the `do_fence` signal inside the core. To work around this issue, we added a FIRRTL transformation that
asserts the `io.busy` signal whenever the core dispatches a instruction to the accelerator, and holds it until the
accelerator actually lowers it. We provide a user level annotation called the `MakeRoCCBusyLatencyInsensitive` which
hints the FIRRTL compiler to perform the pass or not. As you can see in the below example, all you need to do is
wrap the `io.busy`, `io.cmd.ready`, `io.cmd.valid` signals in the `MakeRoCCBusyLatencyInsensitive` scala `Object`.
The compiler will take care of the rest for you!


#### Preemptive assertion of `io.busy` in RoCC
```scala
import midas.targetutils.MakeRoCCBusyLatencyInsensitive

class MyRoCC(opcodes: OpcodeSet)(implicit p: Parameters) extends LazyRoCC(
    opcodes = opcodes, nPTWPorts = 1) {
  override lazy val module = new MyRoCCImp(this)
}

class MyRoCCImp(outer: MyRoCC)(implicit p: Parameters) extends LazyRoCCModuleImp(outer) {
  // Implementation

  MakeRoCCBusyLatencyInsensitive(io.busy, io.cmd.ready, io.cmd.valid)
}
```

#### Example configs & topology for Gemmini

The rest of the steps to pull out Gemmini is the same as pulling out the `RocketTile`. The below shows the build recipes and
the user topology.

```yaml
# config_build_recipes.yaml

firesim_gemmini_rocket_split_soc:
  PLATFORM: f1
  TARGET_PROJECT: firesim
  DESIGN: FireSim
  TARGET_CONFIG: WithDefaultFireSimBridges_WithNoTraceFireSimConfigTweaks_chipyard.GemminiRocketConfig
  PLATFORM_CONFIG: BaseF1Config
  TARGET_SPLIT_CONFIG: SplitOuter-Gemmini-2-Base
  deploy_quintuplet: null
  platform_config_args:
    fpga_frequency: 90
    build_strategy: TIMING
  post_build_hook: null
  metasim_customruntimeconfig: null
  bit_builder_recipe: bit-builder-recipes/f1.yaml

firesim_gemmini_rocket_split_accel:
  PLATFORM: f1
  TARGET_PROJECT: firesim
  DESIGN: FireSim
  TARGET_CONFIG: WithDefaultFireSimBridges_WithNoTraceFireSimConfigTweaks_chipyard.GemminiRocketConfig
  PLATFORM_CONFIG: BaseF1Config
  TARGET_SPLIT_CONFIG: SplitInner-Gemmini-2-0
  deploy_quintuplet: null
  platform_config_args:
    fpga_frequency: 90
    build_strategy: TIMING
  post_build_hook: null
  metasim_customruntimeconfig: null
  bit_builder_recipe: bit-builder-recipes/f1.yaml
```

```python
# user_topology.py

def fireaxe_split_gemmini_from_soc_config(self) -> None:
    hwdb_entries = {
        "firesim_gemmini_rocket_split_soc"   : True,
        "firesim_gemmini_rocket_split_accel" : False
    }
    self.fireaxe_two_node_base_config(hwdb_entries)
```

### Pulling out multiple tiles and grouping them onto multiple FPGAs

Now comes the real power of FireAxe. We can specify the number of FPGAs and the modules that we want to pull out, and FireAxe will partition the modules on to the FPGAs that we specified. For instance, when we want to split the SoC containing 4 `RocketTile`s onto 3 FPGAs, we can each place 2 `RocketTile`s on FPGA-0 and FPGA-1 and place our `root` design on FGPA-2. You can also pull out RoCC accelerators in the same manner.


#### Build recipes for splitting 3 designs

As you can see, we will be splitting out or design into 3 FPGAs.
One important thing to note is that we specified 4 module names that we want to pull out.
`RocketTile.0~3` means that we will be pulling out 4 `RocketTiles` with indexes from 0~3.
The rest of  `TARGET_SPLIT_CONFIG` is the same as usual.
From this, the FIRRTL passes will count the number of module instances, and automatically divide them up onto (`FPGACount` - 1) FPGAs.

```yaml
# config_build_recipes.yaml
firesim_quad_rocket_3fpga_split_soc:
  PLATFORM: f1
  TARGET_PROJECT: firesim
  DESIGN: FireSim
  TARGET_CONFIG: WithDefaultFireSimBridges_WithNoTraceFireSimConfigTweaks_chipyard.QuadCoreRocketConfig
  PLATFORM_CONFIG: BaseF1Config
  TARGET_SPLIT_CONFIG: SplitOuter-RocketTile.0~3-3-Base
  deploy_quintuplet: null
  platform_config_args:
    fpga_frequency: 90
  build_strategy: TIMING
  post_build_hook: null
  metasim_customruntimeconfig: null
  bit_builder_recipe: bit-builder-recipes/f1.yaml

firesim_quad_rocket_3fpga_split_tiles_0:
  PLATFORM: f1
  TARGET_PROJECT: firesim
  DESIGN: FireSim
  TARGET_CONFIG: WithDefaultFireSimBridges_WithNoTraceFireSimConfigTweaks_chipyard.QuadCoreRocketConfig
  PLATFORM_CONFIG: BaseF1Config
  TARGET_SPLIT_CONFIG: SplitInner-RocketTile.0~3-3-0
  deploy_quintuplet: null
  platform_config_args:
    fpga_frequency: 90
  build_strategy: TIMING
  post_build_hook: null
  metasim_customruntimeconfig: null
  bit_builder_recipe: bit-builder-recipes/f1.yaml

firesim_quad_rocket_3fpga_split_tiles_1:
  PLATFORM: f1
  TARGET_PROJECT: firesim
  DESIGN: FireSim
  TARGET_CONFIG: WithDefaultFireSimBridges_WithNoTraceFireSimConfigTweaks_chipyard.QuadCoreRocketConfig
  PLATFORM_CONFIG: BaseF1Config
  TARGET_SPLIT_CONFIG: SplitInner-RocketTile.0~3-3-1
  deploy_quintuplet: null
  platform_config_args:
    fpga_frequency: 90
  build_strategy: TIMING
  post_build_hook: null
  metasim_customruntimeconfig: null
  bit_builder_recipe: bit-builder-recipes/f1.yaml
```

#### User topology for splitting the design 

Now all we need to do is specify the user topology.
The `edges` represents the edges in the partition graph (from the `root` partition to `leaf` partitions),
and the `hwdb_entries` represents the partitioned build recipes. The `fireaxe_split_topology_base_config`
generates the `FireSimPipeNode`s and the `FireSimServerNodes` accordingly.

By setting the `config_runtime[target_config][topology]` to `firesim_split_quad_rocket_tiles_from_soc_3fpga_config`,
you can run partitioned sims by just running `firesim infrasetup && firesim runworkload`!

```python
def fireaxe_split_topology_base_config(self, edges, hwdb_entries) -> None:
    hwdb_to_server_map = {}
    for (hwdb, base) in hwdb_entries.items():
        server = FireSimServerNode(hwdb, partitioned=True, base_partition=base)
        hwdb_to_server_map[hwdb] = server
        rootLogger.info(f"user_topology {server.server_id_internal}")

    self.roots = [FireSimPipeNode() for _ in range(len(edges))]
    for (e, pipe) in zip(edges, self.roots):
        [u, v] = e
        server_edge = [hwdb_to_server_map[u], hwdb_to_server_map[v]]
        if isinstance(pipe, FireSimPipeNode):
            pipe.add_downlink_partition(server_edge)

def firesim_split_quad_rocket_tiles_from_soc_3fpga_config(self) -> None:
    edges = [
        ["firesim_quad_rocket_3fpga_split_soc", "firesim_quad_rocket_3fpga_split_tiles_0"],
        ["firesim_quad_rocket_3fpga_split_soc", "firesim_quad_rocket_3fpga_split_tiles_1"]
    ]

    hwdb_entries = {
        "firesim_quad_rocket_3fpga_split_soc"     : True,  # True indicates that this is a root partition
        "firesim_quad_rocket_3fpga_split_tiles_0" : False, # False indicates that this is a leaf partition
        "firesim_quad_rocket_3fpga_split_tiles_1" : False  # False indicates that this is a leaf partition
      }
    self.fireaxe_split_topology_base_config(edges, hwdb_entries)
```

## Miscellaneous tips
### Minimizing the number of FPGAs
Simulation performance naturally drops as you use more FPGAs. We recommend using 2 FPGAs for now (we observed a 2x simulation
performance drop when expanding from 2 to 3 FPGAs).

### Disabling Tracer-V
FireAxe's performance is directly correlated to the bitwidth of the partitioned interface. To improve simulation performance, 
you can use the `WithNoTracerVConfig` when splitting out tiles to reduce the number of bits going through the tile boundary.



### ChangeLogs
- Indexing changed from [Base, 0, 1, 2, 3] to [0, 1, 2, N-1] where (N-1) is the base SoC
