from __future__ import annotations

from dataclasses import dataclass

from typing import Dict, Any

@dataclass
class TracerVConfig:
    enable: bool
    select: str
    start: str
    end: str
    output_format: str

    def __init__(self, args: Dict[str, Any]) -> None:
        self.enable = args.get('enable', False) == True
        self.select = args.get('selector', "0")
        self.start = args.get('start', "0")
        self.end = args.get('end', "-1")
        self.output_format = args.get('output_format', "0")

@dataclass
class AutoCounterConfig:
    readrate: int

    def __init__(self, args: Dict[str, Any]) -> None:
        self.readrate = int(args.get('read_rate', "0"))

@dataclass
class HostDebugConfig:
    zero_out_dram: bool
    disable_synth_asserts: bool

    def __init__(self, args: Dict[str, Any]) -> None:
        self.zero_out_dram = args.get('zero_out_dram', False) == True
        self.disable_synth_asserts = args.get('disable_synth_asserts', False) == True

@dataclass
class SynthPrintConfig:
    start: str
    end: str
    cycle_prefix: bool

    def __init__(self, args: Dict[str, Any]) -> None:
        self.start = args.get("start", "0")
        self.end = args.get("end", "-1")
        self.cycle_prefix = args.get("cycle_prefix", True) == True

@dataclass
class PartitionConfig:
    batch_size: int
    preserve_target: int

    def __init__(self, args: Dict[str, Any]) -> None:
        self.batch_size = args.get("batch_size", 1)
        self.preserve_target = args.get("preserve_target", 0)

@dataclass
class TipTracingConfig:
    enable: bool
    core_config: str
    worker: str

    def __init__(self, args: Dict[str, Any]) -> None:
        self.enable = args.get('enable', False) == True
        self.core_config = args.get("core", "0,0")
        self.worker = args.get("worker", "interval")
