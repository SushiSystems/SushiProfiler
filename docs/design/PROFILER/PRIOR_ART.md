# Prior art

The citations below were checked against the venue or arXiv page on 2026-10-26. Tool behaviour is
cited from the vendor's documentation page.

## Tools

| Tool | What we take | What we leave |
| --- | --- | --- |
| PyTorch `torch.profiler` on Kineto ([docs](https://docs.pytorch.org/docs/2.14/profiler.html)) | The `wait / warmup / active / repeat` window over step markers; opt-in flags that each add cost; grouping of averages | Python dispatch hooks |
| Holistic Trace Analysis ([repository](https://github.com/facebookresearch/HolisticTraceAnalysis)) | Temporal breakdown; idle time split by cause; short-kernel and launch statistics; trace diff | Multi-rank communication |
| `torch.utils.benchmark` ([docs](https://docs.pytorch.org/docs/2.14/benchmark_utils.html)) | Warm up, synchronise before reading the clock, keep raw times, take the median | |
| XProf and `jax.profiler` ([XProf](https://github.com/openxla/xprof), [JAX](https://docs.jax.dev/en/latest/profiling.html)) | A step-time overview; asynchronous dispatch waited on before a trace ends | gRPC capture servers |
| CUPTI ([docs](https://docs.nvidia.com/cupti/main/main.html)) | Buffered records, sorted and joined after the fact | Vendor lock-in |
| Nsight Systems and Compute ([Systems](https://docs.nvidia.com/nsight-systems/UserGuide/index.html), [Compute](https://docs.nvidia.com/nsight-compute/ProfilingGuide/index.html)) | Host ranges projected onto the device timeline; speed-of-light percentages; per-precision roofline | Kernel replay, which changes clocks and caches |
| intel-llvm XPTI ([framework](https://github.com/intel/llvm/blob/sycl/xptifw/doc/XPTI_Framework.md)), unitrace ([tool](https://github.com/intel/pti-gpu/tree/master/tools/unitrace)) | A stub that costs nothing with no listener; queued, submitted and executing intervals per kernel | Level Zero metrics |
| ONNX Runtime profiling ([docs](https://onnxruntime.ai/docs/performance/tune-performance/profiling-tools.html)) | Each device kernel attached to the host node that launched it | |
| Ultralytics `results.speed` ([docs](https://docs.ultralytics.com/modes/predict/)) | A cheap per-stage time that is always on | |
| Perfetto ([docs](https://perfetto.dev/docs/)) | Opens Chrome trace JSON offline; SQL over a trace | Its protobuf writer, for now |
| SushiEngine `foundation/profiling` (in-house, built 2026-10-09) | The ring, the clock, the TSC calibration, the thread-retirement guards, the zone tree fold, the Chrome writer | What [ENGINE_CHANGES.md](ENGINE_CHANGES.md) replaces |

## Literature

Performance models:

- S. Williams, A. Waterman, D. Patterson. *Roofline: an insightful visual performance model for
  multicore architectures.* CACM 52(4), 2009. [doi](https://doi.org/10.1145/1498765.1498785).
  Attainable FLOP/s is the lesser of the peak and bandwidth times arithmetic intensity; a kernel
  below both ceilings is bound by latency, launch cost or occupancy.
- C. Yang, R. Gayatri, T. Kurth, P. Basu, S. Williams. *An empirical roofline methodology for
  quantitatively assessing performance portability.* P3HPC at SC18, 2018.
  [pdf](https://crd.lbl.gov/assets/Uploads/p3hpc-roofline-final.pdf). Ceilings measured by
  microbenchmark on the device.
- C. Yang, T. Kurth, S. Williams. *Hierarchical Roofline analysis for GPUs.* CUG 2019; CCPE
  32(20), 2020. [doi](https://doi.org/10.1002/cpe.5547). One ceiling per memory level and
  precision.
- N. Ding, S. Williams. *An instruction roofline model for GPUs.* PMBS at SC19, 2019.
  [abstract](https://www.dcs.warwick.ac.uk/pmbs/pmbs19/PMBS19/abstracts/paper2.html). Kernels with
  few FLOPs.
- Y. Wang, C. Yang, S. Farrell. *Hierarchical Roofline performance analysis for deep learning
  applications.* arXiv 2009.05257, 2020. [arXiv](https://arxiv.org/abs/2009.05257). One point per
  kernel, sized by its share of time.
- H. Qi, E. R. Sparks, A. Talwalkar. *Paleo: a performance model for deep neural networks.* ICLR
  2017. [openreview](https://openreview.net/forum?id=SyVVJ85lg). FLOPs and bytes from shapes;
  predicted beside measured.

Training workloads:

- H. Zhu et al. *TBD: Benchmarking and analyzing deep neural network training.* IISWC 2018.
  [arXiv](https://arxiv.org/abs/1803.06905). Device busy time and FP32 utilisation are two numbers.
- H. Zhu, A. Phanishayee, G. Pekhimenko. *Daydream: accurately estimating the efficacy of
  optimizations for DNN training.* USENIX ATC 2020.
  [page](https://www.usenix.org/conference/atc20/presentation/zhu-hongyu).
- G. X. Yu, T. Grossman, G. Pekhimenko. *Skyline: interactive in-editor computational performance
  profiling for deep neural network training.* UIST 2020.
  [arXiv](https://arxiv.org/abs/2008.06798). Time against batch size separates fixed from
  per-sample cost.
- G. X. Yu, Y. Gao, P. Golikov, G. Pekhimenko. *Habitat: a runtime-based computational performance
  predictor for deep neural network training.* USENIX ATC 2021.
  [page](https://www.usenix.org/conference/atc21/presentation/yu).
- M. Jeon, S. Venkataraman, A. Phanishayee et al. *Analysis of large-scale multi-tenant GPU
  clusters for DNN training workloads.* USENIX ATC 2019.
  [page](https://www.usenix.org/conference/atc19/presentation/jeon). Idle device time from input,
  host stalls and synchronisation.
- M. Wang, C. Meng, G. Long et al. *Characterizing deep learning training workloads on
  Alibaba-PAI.* IISWC 2019. [arXiv](https://arxiv.org/abs/1910.05930). Jobs bound by many small
  kernels.
- P. Mattson et al. *MLPerf Training benchmark.* MLSys 2020.
  [arXiv](https://arxiv.org/abs/1910.01500).
- S. Sridharan et al. *Chakra: advancing performance benchmarking and co-design using standardized
  execution traces.* arXiv 2305.14516, 2023. [arXiv](https://arxiv.org/abs/2305.14516).
- C. Curtsinger, E. D. Berger. *Coz: finding code that counts with causal profiling.* SOSP 2015.
  [doi](https://doi.org/10.1145/2815400.2815409).

Kernel analysis:

- K. Zhou, X. Meng, R. Sai, J. Mellor-Crummey. *GPA: a GPU performance advisor based on
  instruction sampling.* CGO 2021. [arXiv](https://arxiv.org/abs/2009.04061).
- K. Zhou, L. Adhianto et al. *Measurement and analysis of GPU-accelerated applications with
  HPCToolkit.* Parallel Computing 108, 2021. [arXiv](https://arxiv.org/abs/2109.06931). Device
  time attributed to the host context that launched it.
- Z. Jia, M. Maggioni, B. Staiger et al. *Dissecting the NVIDIA Volta GPU architecture via
  microbenchmarking.* arXiv 1804.06826, 2018. [arXiv](https://arxiv.org/abs/1804.06826).
- L. Zheng et al. *Ansor: generating high-performance tensor programs for deep learning.* OSDI
  2020. [page](https://www.usenix.org/conference/osdi20/presentation/zheng).

Measurement method:

- T. Hoefler, R. Belli. *Scientific benchmarking of parallel computing systems: twelve ways to
  tell the masses when reporting performance results.* SC 2015.
  [doi](https://doi.org/10.1145/2807591.2807644).
- A. Georges, D. Buytaert, L. Eeckhout. *Statistically rigorous Java performance evaluation.*
  OOPSLA 2007. [pdf](https://dri.es/files/oopsla07-georges.pdf).
- T. Kalibera, R. E. Jones. *Rigorous benchmarking in reasonable time.* ISMM 2013.
  [doi](https://doi.org/10.1145/2464157.2464160).
- T. Mytkowicz, A. Diwan, M. Hauswirth, P. F. Sweeney. *Producing wrong data without doing
  anything obviously wrong!* ASPLOS 2009.
  [page](https://sape.inf.usi.ch/publications/asplos09.html).
