# Thirdparty Libraries

The geometry source requires two thirdparty header-only libraries to have robust support for (potentially enormous) sparse matrix operations that pop up in mesh processing. 

- [Eigen](https://gitlab.org/libeigen/eigen) provides the core sparse matrix implementation.

- [Spectra](https://github.com/yixuan/spectra) provides nifty algorithms on top of Eigen to find smallest or largest eigenvalues and eigenvectors of sparse matrices. 

Both Eigen and Spectra are licensed under the Mozilla Public License 2.0 (MPL2). A copy of the license is made available in this directory. You can read more about the MPL2 at the [FAQ page](https://www.mozilla.org/en-US/MPL/2.0/FAQ/).
