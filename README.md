# LTPSim_project (<ins>L</ins>ow <ins>T</ins>emperature <ins>P</ins>lasma <ins>S</ins>imulation)

## \_ltplib: <ins>L</ins>ow <ins>T</ins>emperature <ins>P</ins>lasma <ins>Lib</ins>rary
The middle-layer framework that provides simple python-interface to construct PiC+MCC simulations (Particles in Cells + Monte-Carlo Collisions).
The library offers a range of functions and primitives to facilitate a wide class of low-temperature plasma problems.
By creating a more abstract and user-friendly interface, \_ltplib aims to enable researchers and engineers to formulate and solve simple & complex PiC+MCC problems in a flexible manner with aid of python.

Features:
- one-, two-, and three-dimension problems with periodic or absorptive boundary conditions;
- high-order form-factors (up to 3);
- explicit or semi-implicit particle movers;
- fast and flexible Monte-Carle module allowing to simulate arbitrary mixture of active and background components, taking into account anisotropic scattering.

> [!NOTE] The current version of \_ltplib does not provide build-in field solvers, external one should be used.

The code is based on former Θ-Hall [^chernyshev2019], [^chernyshev2022], but it was heavily modified and rewritten from scratch.

[^chernyshev2019]: https://doi.org/10.1088/1361-6463/ab35cb

[^chernyshev2022]: https://doi.org/10.1088/1361-6595/ac4179

## Build instructions
The framework uses [pybind11](https://github.com/pybind/pybind11) to create transparent interface between python and c++ codes. Dependencies will be downloaded automatically by CMake FetchContent. To build the code just run the following statement
```sh
mkdir _ltplib/build && cd _ltplib/build && cmake .. && cmake --build .
```
As a result, two libraries will be generated:
1. `_ltplib/build/src/_ltplib.so`, is the framework itself;
1. `_ltplib/build/src/_default/_default.so`, the backend library containing OpenMP-based solvers.

Native python-way installation via pip is not supported yet, so just copy both binaries in your project's directory. Is is highly recommended to have [numpy](https://github.com/numpy/numpy) installed. It is not necessary to run \_ltplib, but [numpy.ndarray](https://numpy.org/doc/stable/reference/arrays.ndarray.html) is used as a main interface between userspace python-code and \_ltplib.
Build-in documentation is available as follows:
```python
import _ltplib as ltp
help(ltp)
```
The following sections provide a brief overview for \_ltplib components.

## Main classes
### `_ltplib.grid`
Gird is a primary class for every simulation. It describes geometry of the problem, boundary conditions, and spatial-decomposition for parallel computation. The code uses sightly modified approach of tile-decomposition described before in [^decyk2014], [^decyk2015]. The class constructor accepts following arguments:
1. *nd* -- number of spatial dimensions;
1. *step* -- list containing spatial steps along the each axis;
1. *axes* -- list describing spatial decomposition along the each axis;
1. *nodes* -- list containing mapping for computing nodes.

The next example shows how `_ltplib.grid` can be constructed:
```python
grid_cfg = [
 "nd" : 2,
 # First, let's define spatial-step for each dimension
 "step" : [0.25, 0.25], # dx, dy

 # The next two sections describe domain decomposition.
 # Here, the numbers inbetween define edges of sub-domains:
 "axes": [
  # x-axis, 2 slices
  [0, 16, 32],
  # y-axis, 3 slices
  [0, 20, 40, 60], 
 ],
 # Now the position of each sub-domain is described relative to the "axes":
 "nodes": [
  (0, 0), # (0 <= x/dx < 16), (0  <= y/dy < 20)
  (0, 1), # (0 <= x/dx < 16), (20 <= y/dy < 40)
  ...
 ],
 # The links between the nodes will be builded automatically.
 
 # It is possible to mark some points as a particle absorbers
 # using following optional parameter:
 "mask" : [...], # uint8 numpy array, with the same shape as grid axes.
 # Any value != 0 will be considered as adsorbing wall.
 
 # For periodic boundary condition(s) axis(ex) should be marked:
 "loopax": "x",
 
 #For 2d problems with axial-symmetry x-axis should be marked:
 "cylcrd": "x",
]

grid = ltp.grid(**grid_cfg)
```

[^decyk2014]: https://doi.org/10.1016/j.cpc.2013.10.013
[^decyk2015]: https://doi.org/10.1109/MCSE.2014.131

### `_ltplib.pstore`
This class is used to store pVDF samples (macro-particles). The class constructor accepts following arguments:
1. *grid* -- existing grid;
1. *ptinfo* -- description of active components to store;
1. *npmax* -- the capacity (maximum number of samples per node);
1. *nargs* -- (optional, `1+grid.nd+3` by default) number of components.

See the example:
```python
pstore_cfg = [
 "ptinfo" : [
  {"KEY":"e",   "CHARGE/MASS": -5.272810e+17}, # electron
  {"KEY":"Ar+", "CHARGE/MASS": +7.240801e+12}, # argon ion
 ],
 "npmax" : 100000, # the limit is 16777216 samples/node
 "nargs" : 1+2*(grid.nd+3), # in case of using with implicit mover
]

pstore = ltp.pstore(grid, **pstore_cfg)
```

To load samples into the class `pstore.inject` method should be called. Method accepts dictionary, where keys correspond to `"ptinfo"`, and values are numpy arrays to load. The shape of input array should match `[npp, grid.nd+3]`, where `npp` is the number of samples to add. The components of sample's vector are $\{x\,\dots\,v_x\,v_y\,v_z\}$.

### `_ltplib.vcache`
This class is used as a universal node-local cache for grid-based values. For example, it can be used to store electromagnetic field, pVDF moments, background densities, collision frequencies.
Class constructor accepts following arguments:
1. *grid* --- existing grid.
1. *dtype* --- the string describing type (`"f32"` or `"u32"`).
1. *vsize* --- number of components per grid unit (optional, default `1`).
1. *order* --- form-factor's order (optional, default `0`).

### `_ltplib.csection_set`
This class stores cross-section database for Monte-Carlo simulation.
Input cross-sections can be defined by the function or by the points.
In both cases they will be recalculated into cumulative rates and cached into the lookup-table on log-scaled energy-grid $\varepsilon = \varepsilon_{\rm th} + 2^{j/2-4}-0.0625,~j\in\mathbb{N}$, where $\varepsilon_{\rm th}$ is reaction's threshold. This allows to store large amount of cross-sections in a very compact way. Constructor parameters for the class are:
1. *cfg* --- configuration sequence (see below);
1. *max_energy* --- energy limit defining lookup-table's size;
1. *ptdescr* --- string containing keys for active components, separated by spaces.
This argument **must** exactly match with components from `pstore`.
1. *bgdescr* --- string containing keys for background components, separated by spaces.
Optional argument, if given the class will ignore all other backgrounds from configuration sequence.

There are optional keywords arguments:
- *rescale* -- global scale factor for cross-section value.
- *exterp* -- global extrapolation factor (see below).

#### Configuration sequence
All processes are described in a list of dict-entries.
There are three types of entries. Firstly, the active component should be selected (`"TYPE":"PARTICLE"`).
For example
```python
{"TYPE":"PARTICLE", "KEY":"e",
 "ENCFFT": 2.842815e-16, # coefficient to transform speed^2 -> energy
},
```
Secondly, the background should be described (`"TYPE":"BACKGROUND"`)
```python
{"TYPE":"BACKGROUND", "KEY":"CH4",
 "MASSRATE":3.420074282530393e-05, # m/(m+M) coefficient
},
```
> [!NOTE]
> The current version of \_ltplib doesn't support flux & thermal thermal velocities for the background.
> Such functionality will be added later. Now it is considered frozen in laboratory frame of reference.

Processes' description follow next. For these entries `"TYPE"` could be
- `"ELASTIC"`
- `"EXCITATION"`
- `"VIBRATIONAL"`
- `"ROTATIONAL"`
- `"DISSOCIATION"`
- `"IONIZATION"`
- `"ATTACHMENT"`

For inelastic processes energy threshold $\varepsilon_{\rm th}$ should be defined by field `"THRESHOLD"`.

Then the first set of processes is finished, the next background (or active particle) can be selected.
> [!TIP]
> It is allowed for same `"TYPE":"PARTICLE"` to repeat for each background.
> Extra entries will be ignored.
> It was made this way because it is pretty useful to store the conf.seq. as a preset for the each type of the gas.

There are two forms of how process cross-section can be passed:
directly `"CSEC": cs_data` or as a tuple with additional parameters
`"CSEC" : (cs_data, {"scale" : 1e4, "exterp": 1})`.
The *scale* parameter works together with *rescale*, and *exterp* overwrites the global value.
Cross-section can be defined analytically as a python-function of two arguments
$\sigma(\varepsilon,\,\varepsilon_{\rm th})$, for example:
```python
 "CSEC" : lambda en, th: 1e-15*(en-th)/((en-th)**2+1),
```
Or else, cross-section can be defined by list of points:
```python
 "CSEC" : [
  (1.20e+01,1.00e-21),(2.00e+01,1.00e-20),(4.00e+01,1.70e-20),
  (6.00e+01,1.80e-20),(8.00e+01,1.70e-20),(1.00e+02,1.40e-20),
  (2.00e+02,1.00e-20),(6.00e+02,5.00e-21),(1.10e+03,2.00e-21),
 ],
```
It is possible to read points from the text file:
```python
 "CSEC": "CH4_momentum_CommunityDB.txt",
```
> [!TIP]
> Lines started with ``'#'``, ``'!'``, ``'%'``, ``'//'`` are ignored.

If it is necessary, first line to start scanning can be specified by parameter *lineno*.
It is also possible to read data directly from [LXCat](https://lxcat.net)-file using *search*-parameter.
In this case scan will start from the desired string and points will be readed from the nearest data-block nestled between `"-----"`-lines.

Log-log interpolation is used to map points into lookup-tale
$\sigma(\varepsilon) = \exp(a \log\varepsilon)\cdot b$.
If extrapolation parameter (*exterp*) is bigger than zero
and condition $\partial\sigma/\partial\varepsilon$ is fulfilled for high-energies, then extrapolation will be build.
Parameter *exterp* controls the range of energies to build extrapolation
(*exterp* = $\log_{10}\max(\varepsilon)-\log_{10}\varepsilon$).
Default value is 0.25, *exterp* = 0 turns off extrapolation.

#### Approximation for anisotropic scattering
In general, scattering is described by differential cross-section
$\sigma(\varepsilon,\ \alpha)$, where
$\sigma(\varepsilon)
=2\pi\int_0^\pi \sin\alpha\ \sigma(\varepsilon,\ \alpha)\ {\rm d}\alpha$,
$\alpha$ -- azimuthal scattering angle.
Framework \_ltplib includes first-order approximation for $\sigma(\varepsilon,\ \alpha)$ uning momentum-transfer cross-section:
$$
	\sigma_{\rm m} = 2\pi\int_{0}^{\pi}
	\left[1-\cos\alpha\sqrt{1-\frac{\varepsilon_{\rm th}}{\varepsilon}}\right]
	\sigma(\varepsilon,\,\alpha)\ {\rm d}\alpha.
$$
Internally, fitting-parameter $\xi(\varepsilon)$ [^janssen2016] ,[^flynn2024] is used:
$$
	\frac{\sigma_{\rm m}}{\sigma}
	= 1+\sqrt{1-\frac{\varepsilon_{\rm th}}{\varepsilon}} \cdot
	\left[
		1 - \frac{1-\xi}{\xi^2} \left( \frac{1+\xi}{2} \log\left(\frac{1+\xi}{1-\xi}\right) - \xi\right)
	\right].
$$
By default, scattering considered to be isotropic, i.e.
$\xi\equiv 0$ and $\sigma_{\rm m}=\sigma$.
For $\xi \rightarrow +1$ small-angle collisions dominate (forward-scattering),
$\xi \rightarrow -1$ correspond to large-angle collisions (back-scattering).
> [!NOTE] $\sigma_{\rm m}/\sigma \leq 2$.

There are three ways to define anisotropic scattering:
- pass total (`"CSEC"`) + momentum-transfer cross-section (field `"MTCS"`, the syntax is identical to `"CSEC"`);
- pass total (`"CSEC"`) cross-section + fitting parameter $\xi(\varepsilon - \varepsilon_{\rm th})$ as a python-function (field `"DCSFN"`);
- or pass `"MTCS"` + `"DCSFN"`.

[^janssen2016]: https://doi.org/10.1088/0963-0252/25/5/055026

[^flynn2024]: https://doi.org/10.1088/1361-6463/ad3477

#### Ionization
In case of ionization, it is assumed that there is not impulse transfer between incident electron and heavy particle ($m/M$-term is ignored).
The energy/impulse-balance is determined only by incident and secondary particle(s).
This division is arbitrary, we consider particle secondary if it has smaller resulting energy, i.e. $\varepsilon_2<\varepsilon_1$.
From energy conservation
$$
\varepsilon' = \varepsilon-\varepsilon_{\rm th} = \varepsilon_1 + \varepsilon_2.
$$
And from impulse conservation
$$
\begin{array}{lll}
	\cos \alpha_1 & = & \sqrt{\varepsilon_1/\varepsilon'}
	\\
	\cos \alpha_2 & = & \sqrt{\varepsilon_2/\varepsilon'}
	\\
	\beta_1+\pi   & = & \beta_2 ~ \textrm{(polar scattering angles)}
\end{array}
$$
As a result, ionization collisions are always considered anisotropic.

The energy-spectrum for secondary electrons uses Opal-Peterson-Beaty approximation (OPB-approximation, [^opal1971], [^opal1972]).
The spectrum is defined by a single parameter
$\varepsilon_{\rm OPB}\sim\varepsilon_{\rm th}$ (field `"OPBPARAM"`).
If it is not given, $\varepsilon_{\rm th}$ will be used instead.

> [!NOTE] 
> The current implementation is unfinished and doesn't allow to spawn multiple electrons or ions.
> This functionality will be added in further versions.

[^opal1971]: https://doi.org/10.1063/1.1676707

[^opal1972]: https://doi.org/10.1016/s0092-640x(72)80004-4

#### Examples
1. Elastic collision, anisotropic scattering defined by $\sigma_{\rm m}$:
```python
{"TYPE":"ELASTIC",
 "CSEC":("H2O/ELASTIC-ICS.song2021.txt"
 , {"scale":1e-16}),
 "MTCS":("H2O/ELASTIC-MTCS.song2021.txt"
 , {"scale":1e-16}),
},
```
2. Electron excitation, anisotropic scattering defined by $\xi(\varepsilon - \varepsilon_{\rm th})$:
```python
{"TYPE":"EXCITATION", "THRESHOLD":8.0,
 "CSEC":  lambda en, th=8.0: 1e-17*(en-th)/((en-th)**(7/4)+1),
 "DCSFN": lambda en, es=4.5: (4*en/es)/(1+4*en/es), # screened Coulomb
},
```
3. Excitation of rotation-level (reading from LXCat, disable extrapolation)
```python
{"TYPE":"ROTATIONAL", "THRESHOLD":4.0e-2,
 "CSEC":("H2O.txt"
 , {"search":"H2O -> H2O(ROT)", "exterp":0}),
},
```
4. Ionization
```python
{"TYPE":"IONIZATION", "THRESHOLD":12.61, "OPBPARAM":13.0,
 "CSEC":(f"{fpath}/CH4_N2_O2_H2O.txt"
 , {"search":"PARAM.:  E = 12.61 eV, complete set")),
},
```

## Function bindings
(To be done...)
