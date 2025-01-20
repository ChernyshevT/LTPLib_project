# \_ltplib: <ins>L</ins>ow <ins>T</ins>emperature <ins>P</ins>lasma <ins>Lib</ins>rary
The middle-layer framework that provides simple python-interface to construct PiC+MCC simulations (Particles in Cells + Monte-Carlo Collisions).
The library offers a range of functions and primitives to facilitate a wide class of low-temperature plasma problems.
By creating a more abstract and user-friendly interface, \_ltplib aims to enable researchers and engineers to formulate and solve simple & complex PiC+MCC problems in a flexible manner with aid of python.

Features:
- one-, two-, and three-dimension problems with periodic or absorptive boundary conditions;
- high-order form-factors (up to 3);
- explicit or semi-implicit particle movers;
- fast and flexible Monte-Carle module allowing to simulate arbitrary mixture of active and background components, taking into account anisotropic scattering.
> [!NOTE]
> The current version of \_ltplib does not provide build-in field solvers, external one should be used.

The code is based on former Θ-Hall [^chernyshev2019], [^chernyshev2022], but it was heavily modified and rewritten from scratch.

[^chernyshev2019]: Chernyshev, T., Son, E., & Gorshkov, O. (2019). _2D3V kinetic simulation of Hall effect thruster, including azimuthal waves and diamagnetic effect_. In Journal of Physics D: Applied Physics (Vol. 52, Issue 44, p. 444002). IOP Publishing. 
[DOI:10.1088/1361-6463/ab35cb](https://doi.org/10.1088/1361-6463/ab35cb)

[^chernyshev2022]: Chernyshev, T., & Krivoruchko, D. (2022). _On a force balance and role of cathode plasma in Hall effect thrusters._ In Plasma Sources Science and Technology (Vol. 31, Issue 1, p. 015001). IOP Publishing.
[DOI:10.1088/1361-6595/ac4179](https://doi.org/10.1088/1361-6595/ac4179)

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
### `_ltplib.grid` (problem's geometry)
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

[^decyk2014]: Decyk, V. K., & Singh, T. V. (2014). _Particle-in-Cell algorithms for emerging computer architectures._ In Computer Physics Communications (Vol. 185, Issue 3, pp. 708–719). Elsevier BV. 
[DOI:10.1016/j.cpc.2013.10.013](https://doi.org/10.1016/j.cpc.2013.10.013)

[^decyk2015]: Decyk, V. K. (2015). _Skeleton Particle-in-Cell Codes on Emerging Computer Architectures._ In Computing in Science & Engineering (Vol. 17, Issue 2, pp. 47–52). Institute of Electrical and Electronics Engineers (IEEE).
[DOI:10.1109/mcse.2014.131](https://doi.org/10.1109/MCSE.2014.131)

### `_ltplib.pstore` (particle storage)
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

### `_ltplib.vcache` (value cache)
This class is used as a universal node-local cache for grid-based values. For example, it can be used to store electromagnetic field, pVDF moments, background densities, collision frequencies.
Class constructor accepts following arguments:
1. *grid* --- existing grid.
1. *dtype* --- the string describing type (`"f32"` or `"u32"`).
1. *vsize* --- number of components per grid unit (optional, default `1`).
1. *order* --- form-factor's order (optional, default `0`).

### `_ltplib.csection_set` (cross-section set)
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
$\sigma(\varepsilon,\ \varepsilon_{\rm th})$, for example:
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

Log-log interpolation is used to map points into lookup-table
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
$\alpha$ -- azimuthal scattering angle (relative to the incident direction).
Framework \_ltplib includes first-order approximation for $\sigma(\varepsilon,\ \alpha)$ uning momentum-transfer cross-section:
```math
	\sigma_{\rm m} = 2\pi\int_{0}^{\pi}
	\left[1-\cos\alpha\sqrt{1-\frac{\varepsilon_{\rm th}}{\varepsilon}}\right]
	\sigma(\varepsilon,\ \alpha)\ {\rm d}\alpha.
```
Internally, fitting-parameter $\xi(\varepsilon)$ [^janssen2016] ,[^flynn2024] is used:
```math
	\frac{\sigma_{\rm m}}{\sigma}
	= 1+\sqrt{1-\frac{\varepsilon_{\rm th}}{\varepsilon}} \cdot
	\left[
		1 - \frac{1-\xi}{\xi^2} \left( \frac{1+\xi}{2} \log\left(\frac{1+\xi}{1-\xi}\right) - \xi\right)
	\right].
```
By default, scattering considered to be isotropic, i.e.
$\xi\equiv 0$ and $\sigma_{\rm m}=\sigma$.
For $\xi \rightarrow +1$ small-angle collisions dominate (forward-scattering),
$\xi \rightarrow -1$ correspond to large-angle collisions (back-scattering).
> [!NOTE] 
> $\sigma_{\rm m}/\sigma \leq 2$.

There are three ways to define anisotropic scattering:
- pass total (`"CSEC"`) + momentum-transfer cross-section (field `"MTCS"`, the syntax is identical to `"CSEC"`);
- pass total (`"CSEC"`) cross-section + fitting parameter $\xi(\varepsilon - \varepsilon_{\rm th})$ as a python-function (field `"DCSFN"`);
- or pass `"MTCS"` + `"DCSFN"`.

[^janssen2016]: Janssen, J. F. J., Pitchford, L. C., Hagelaar, G. J. M., & van Dijk, J. (2016). _Evaluation of angular scattering models for electron-neutral collisions in Monte Carlo simulations._ In Plasma Sources Science and Technology (Vol. 25, Issue 5, p. 055026). IOP Publishing. 
[DOI:10.1088/0963-0252/25/5/055026](https://doi.org/10.1088/0963-0252/25/5/055026)

[^flynn2024]: Flynn, M., Vialetto, L., Fierro, A., Neuber, A., & Stephens, J. (2024). _Benchmark calculations for anisotropic scattering in kinetic models for low temperature plasma._ In Journal of Physics D: Applied Physics (Vol. 57, Issue 25, p. 255204). IOP Publishing.
[DOI:10.1088/1361-6463/ad3477](https://doi.org/10.1088/1361-6463/ad3477)

#### Ionization
In case of ionization, it is assumed that there is not impulse transfer between incident electron and heavy particle ($m/M$-term is ignored).
The energy/impulse-balance is determined only by incident and secondary particle(s).
This division is arbitrary, we consider particle secondary if it has smaller resulting energy, i.e. $\varepsilon_2<\varepsilon_1$.
From energy and impulse conservation
```math
\begin{align}
	& \varepsilon_1 + \varepsilon_2 & = & \varepsilon-\varepsilon_{\rm th}
	\\
	& \cos \alpha_1 & = & \sqrt{\varepsilon_1\over\varepsilon-\varepsilon_{\rm th}}
	\\
	& \cos \alpha_2 & = & \sqrt{\varepsilon_2\over\varepsilon-\varepsilon_{\rm th}}
	\\
	& \beta_1+\pi & = & \beta_2,
\end{align}
```
where $\beta_{1}$ & $\beta_{2}$ are polar scattering angles.
As a result, ionization collisions are always considered anisotropic.

The energy-spectrum for secondary electrons uses Opal-Peterson-Beaty approximation (OPB-approximation, [^opal1971], [^opal1972]).
The spectrum is defined by a single parameter
$\varepsilon_{\rm OPB}\sim\varepsilon_{\rm th}$ (field `"OPBPARAM"`).
If it is not given, $\varepsilon_{\rm th}$ will be used instead.
> [!NOTE] 
> The current implementation is unfinished and doesn't allow to spawn multiple electrons or ions.
> This functionality will be added in further versions.

[^opal1971]: Opal, C. B., Peterson, W. K., & Beaty, E. C. (1971). _Measurements of Secondary-Electron Spectra Produced by Electron Impact Ionization of a Number of Simple Gases._ In The Journal of Chemical Physics (Vol. 55, Issue 8, pp. 4100–4106). AIP Publishing.
[DOI:10.1063/1.1676707](https://doi.org/10.1063/1.1676707)

[^opal1972]: Opal, C. B., Beaty, E. C., & Peterson, W. K. (1972). _Tables of secondary-electron-production cross sections._ In Atomic Data and Nuclear Data Tables (Vol. 4, pp. 209–253). Elsevier BV. 
[DOI:10.1016/s0092-640x(72)80004-4](https://doi.org/10.1016/s0092-640x(72)80004-4)

#### Entries examples
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

### `_ltplib.bind_ppush_fn` (motion equation solver)
This function binds its' arguments to motion equation solver from the backend.
The function accepts the following arguments:
- *pstore* --- pVDF samples;
- *descr* --- string containing components of electromagnetic field
and type of the solver separated by semicolon symbol (for example `"Ex Ey Bz : METHOD"`);
- *emfield* --- value cache for electromagnetic field (`dtype="f32"`).

Resulting functional object has following signature
`(dt : float) -> _ltplib.RET_ERRC`,
where `dt` is time step. Two solvers are available.

#### Explicit scheme
This is 2nd-order integrator utilizes Leap-Frog algorithm with Boris splitting scheme [^birdsall2018].
This scheme is is widely known and it is *de-facto standard* in context of plasma simulation. 
In this scheme samples' coordinates and velocities are shifted by $\delta t/2$:
```math
	\left\{\begin{align}
	& \vec{ v}(t+{\delta t/2})
	& =
	& \vec{ v}(t-{\delta t/2}) + \delta t\ \vec{ a}(t)
	\\
	& \vec{ r}(t+\delta t)
	& =
	& \vec{ r}(t) + \delta t\ \vec{ v}(t+{\delta t/2}).
	\end{align}\right.
```
The scheme is encoded by `"LEAPF"`-keyword.

[^birdsall2018]: Birdsall, C. K., & Langdon, A. B. (2018). _Plasma Physics via Computer Simulation._ CRC Press.
[DOI:10.1201/9781315275048](https://doi.org/10.1201/9781315275048)

#### Semi-implicit scheme
This 2nd-order scheme was introduced by Borodachev and Kolomiets [^borodachev2011].
Samples' coordinates and velocities are synchronous in this scheme:
```math
	\left\{\begin{align}
	& \vec{ v}(t+\delta t)
	& =
	& \vec{ v}(t)
	+ {\delta t/2}\ \left[\vec{ a}(t) + \vec{ a}(t+\delta t)\right]
	\\
	& \vec{ r}(t+\delta t)
	& =
	& \vec{ r}(t)
	+ {\delta t/2}\ \left[\vec{ v}(t)+\vec{ v}(t+\delta t)\right].
	\end{align}\right.
```
The scheme is solvable for $\vec{ v}(t+\delta t)$-term [^tajima2018-book].
As a result, only $\vec{ E}$ & $\vec{ B}$ fields at time moment $t+\delta t$ are unknown.
The system can be solved as iterative predictor-corrector process.
- Initial rough approximation (predictor step) assumes $\vec{ a}(t+\delta t) = \vec{ a}(t)$.
- Following corrector step adjusts approximation using updated field values.

Usually, $\lesssim 3$ additional iterations are enough to minimize  an error of closure.
Our implementation caches $t$-moment field contribution for the each sample, so one should set double *nargs*-parameter for *pstore*.
> [!NOTE]
> This scheme is not supported for cylindrical geometry, yet.

[^borodachev2011]: Borodachev, L. V., & Kolomiets, D. O. (2011). _Calculation of particle dynamics in the nonradiative model of plasma._ In Mathematical Models and Computer Simulations (Vol. 3, Issue 3, pp. 357–364). Pleiades Publishing Ltd.
[DOI:10.1134/s2070048211030045](https://doi.org/10.1134/S2070048211030045)

[^tajima2018-book]: Tajima, T. (2018). _Computational Plasma Physics._ CRC Press. 
[DOI:10.1201/9780429501470](https://doi.org/10.1201/9780429501470)

### `_ltplib.bind_order_fn`
After the run of `ppush_fn` the coherence of *pstore* is violated because of samples leaving the nodes.
This function is used to create the binding to restore the coherence.
The function accepts only one argument:
- *pstore* --- pVDF samples.

Resulting functional object has following signature
`() -> _ltplib.RET_ERRC`.

### `_ltplib.bind_ppost_fn` (obtain pVDF moments)
This binding is used to calculate raw pVDF moments [^saint-raymond2009]:
- concentration
$n = \int_\vec{ v}f(\vec{ r},\ \vec{ v})\ {\rm d}\vec{ v}$;
- flux vector
$\vec{ v} = \int_\vec{ v}\vec{ v}\ f(\vec{ r},\ \vec{ v})\ {\rm d}\vec{ v}$;
- pressure/stress tensor
${\rm p} = \int_\vec{ v}\vec{ v}\otimes\vec{ v}\ f(\vec{ r},\ \vec{ v})\ {\rm d}\vec{ v}$.

The function accepts the following arguments:
- *pstore* --- pVDF samples;
- *ptfluid* --- value cache for the result (`dtype="f32"`);
- *mode* --- string describing moments to calculate:
	- `"C"` --- concentration;
	- `"CF"` --- concentration, flux;
	- `"CFP"` --- concentration, flux, pressure ($\vec{ p}_{ij,\ i = j}$);
	- `"CFPS"` --- concentration, flux, pressure, stress (${\rm p}_{ij,\ i \ne j}$).

Resulting functional object has following signature
`() -> _ltplib.RET_ERRC`.

[^saint-raymond2009]: Saint-Raymond, L. (2009). _Hydrodynamic Limits of the Boltzmann Equation._ In Lecture Notes in Mathematics. Springer Berlin Heidelberg.
[DOI:10.1007/978-3-540-92847-8](https://doi.org/10.1007/978-3-540-92847-8)

### `_ltplib.bind_remap_fn`
This binding is used to transfer data between value cache and numpy array.
The function accepts the following arguments:
- *vcache* --- value cache (local data);
- *direction* --- string;
- *iodata* --- numpy array (global data).
```python
_ltplib.bind_remap_fn(vcache, "<", iodata) # to copy from iodata to vcache
_ltplib.bind_remap_fn(vcache, ">", iodata) # to copy from vcache to iodata 
```
Functional object's signature is `() -> ()`.

### `_ltplib.bind_mcsim_fn` (collision simulation)
This binding is used to perform Monte-Carlo simulation.
The arguments are:
- *pstore* --- pVDF samples;
- *cfreq* --- value cache to count successful collision events (`dtype="u32"`, `order=0`);
- *cset* --- cross-sections set;
- *bgrnd* --- value cache containing background densities (`dtype="f32"`, `order=0`).

Functional object's signature is
`(dt: float, seed: int) -> _ltplib.RET_ERRC`,
where `dt` is time step and `seed` is random number.

#### Search algorithm

(To be done...)

# Code examples for \ltplib

(To be done...)
