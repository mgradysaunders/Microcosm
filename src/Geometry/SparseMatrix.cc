#include "Microcosm/Geometry/SparseMatrix"
#include "Microcosm/utility"

#include "Eigen/Sparse"
#include "Spectra/GenEigsSolver.h"
#include "Spectra/MatOp/SparseCholesky.h"
#include "Spectra/MatOp/SparseGenMatProd.h"
#include "Spectra/MatOp/SparseSymMatProd.h"
#include "Spectra/SymEigsSolver.h"
#include "Spectra/SymGEigsSolver.h"

namespace mi::geometry {

void SparseMatrix::resize(size_t numRows, size_t numCols) {
  mShape.resize(numRows, numCols);
  for (auto itr = mValues.begin(); itr != mValues.end();) {
    if (
      itr->first[0] >= numRows || //
      itr->first[1] >= numCols)
      itr = mValues.erase(itr);
    else
      ++itr;
  }
}

void SparseMatrix::setZero() { mValues.clear(); }

void SparseMatrix::setRowToZero(size_t i) {
  for (auto itr = mValues.lower_bound({i, 0}); itr != mValues.end() && itr->first[0] == i;) itr = mValues.erase(itr);
}

void SparseMatrix::setColToZero(size_t j) {
  for (auto itr = mValues.begin(); itr != mValues.end();) {
    if (itr->first[1] == j)
      itr = mValues.erase(itr);
    else
      ++itr;
  }
}

void SparseMatrix::setIdentity() {
  setZero();
  for (size_t k = 0; k < min(rows(), cols()); k++) operator()(k, k) = 1;
}

void SparseMatrix::addIdentity(double factor) {
  for (size_t k = 0; k < min(rows(), cols()); k++) operator()(k, k) += factor;
}

double SparseMatrix::getValue(IndexVector<2> ij) const {
  auto itr = mValues.find(ij);
  if (itr != mValues.end()) return itr->second;
  return 0;
}

double SparseMatrix::setValue(IndexVector<2> ij, double value) {
  auto itr = mValues.lower_bound(ij);
  if (itr == mValues.end() || itr->first != ij) {
    if (value != 0) mValues.try_emplace(itr, ij, value);
    return 0;
  }
  double prevValue = itr->second;
  if (value == 0)
    mValues.erase(itr);
  else
    itr->second = value;
  return prevValue;
}

SparseMatrix &SparseMatrix::operator+=(const SparseMatrix &other) {
  for (const auto &[key, value] : other) mValues[key] += value;
  return *this;
}

SparseMatrix &SparseMatrix::operator-=(const SparseMatrix &other) {
  for (const auto &[key, value] : other) mValues[key] -= value;
  return *this;
}

SparseMatrix &SparseMatrix::operator*=(double factor) {
  for (auto &[key, value] : *this) value *= factor;
  return *this;
}

SparseMatrix &SparseMatrix::operator/=(double factor) {
  for (auto &[key, value] : *this) value /= factor;
  return *this;
}

SparseMatrix SparseMatrix::transpose() const {
  SparseMatrix result(this->cols(), this->rows());
  for (const auto &[ij, valueIJ] : *this) result(ij[1], ij[0]) = valueIJ;
  return result;
}

SparseMatrix SparseMatrix::dot(const SparseMatrix &other) const {
  SparseMatrix result(this->rows(), other.cols());
  for (const auto &[ik, valueIK] : *this) {
    for (size_t j = 0; j < other.cols(); j++) {
      if (double value = valueIK * other.getValue({ik[1], j}); value != 0) {
        result(ik[0], j) += value;
      }
    }
  }
  return result;
}

[[nodiscard]] static auto convertToEigen(const Matrixd &matrix) {
  Eigen::MatrixXd result{matrix.rows(), matrix.cols()};
  for (size_t i = 0; i < matrix.rows(); i++)
    for (size_t j = 0; j < matrix.cols(); j++) result(i, j) = double(matrix(i, j));
  return result;
}

[[nodiscard]] static auto convertToEigen(const SparseMatrix &matrix) {
  std::vector<Eigen::Triplet<double>> triplets;
  triplets.reserve(matrix.numNonZero());
  for (auto &[ij, value] : matrix) triplets.emplace_back(Eigen::Index(ij[0]), Eigen::Index(ij[1]), double(value));
  Eigen::SparseMatrix<double> result(matrix.rows(), matrix.cols());
  result.setFromTriplets(triplets.begin(), triplets.end());
  return result;
}

[[nodiscard]] static auto convertBackFromEigen(const Eigen::VectorXd &vector) noexcept {
  Vectord result{with_shape, vector.size()};
  for (size_t i = 0; i < result.size(); i++) result[i] = double(vector[i]);
  return result;
}

[[nodiscard]] static auto convertBackFromEigen(const Eigen::VectorXcd &vector) noexcept {
  Vectorcd result{with_shape, vector.size()};
  for (size_t i = 0; i < result.size(); i++) result[i] = complex<double>(vector[i]);
  return result;
}

[[nodiscard]] static auto convertBackFromEigen(const Eigen::MatrixXd &matrix) noexcept {
  Matrixd result{with_shape, matrix.rows(), matrix.cols()};
  for (size_t i = 0; i < result.rows(); i++)
    for (size_t j = 0; j < result.cols(); j++) result(i, j) = double(matrix(i, j));
  return result;
}

[[nodiscard]] static auto convertBackFromEigen(const Eigen::MatrixXcd &matrix) noexcept {
  Matrixcd result{with_shape, matrix.rows(), matrix.cols()};
  for (size_t i = 0; i < result.rows(); i++)
    for (size_t j = 0; j < result.cols(); j++) result(i, j) = complex<double>(matrix(i, j));
  return result;
}

[[nodiscard]] static const char *infoToString(Eigen::ComputationInfo info) noexcept {
  if (info == Eigen::NumericalIssue) return "NumericalIssue";
  if (info == Eigen::NoConvergence) return "NoConvergence";
  if (info == Eigen::InvalidInput) return "InvalidInput";
  return "Success";
}

[[nodiscard]] static const char *infoToString(Spectra::CompInfo info) noexcept {
  if (info == Spectra::CompInfo::NotComputed) return "NotComputed";
  if (info == Spectra::CompInfo::NotConverging) return "NotConverging";
  if (info == Spectra::CompInfo::NumericalIssue) return "NumericalIssue";
  return "Success";
}

Matrixd SparseMatrix::solveQR(const Matrixd &matrixB) const {
  Eigen::SparseMatrix matrixA{convertToEigen(*this)};
  Eigen::SparseQR<Eigen::SparseMatrix<double>, Eigen::COLAMDOrdering<int>> decompA{matrixA};
  if (decompA.info() != Eigen::Success) throw Error(std::runtime_error("Sparse QR decomposition failed! ({})"_format(infoToString(decompA.info()))));
  return convertBackFromEigen(Eigen::MatrixXd{decompA.solve(convertToEigen(matrixB))});
}

Matrixd SparseMatrix::solveLU(const Matrixd &matrixB) const {
  Eigen::SparseMatrix matrixA{convertToEigen(*this)};
  Eigen::SparseLU decompA{matrixA};
  if (decompA.info() != Eigen::Success) throw Error(std::runtime_error("Sparse LU decomposition failed! ({})"_format(infoToString(decompA.info()))));
  return convertBackFromEigen(Eigen::MatrixXd{decompA.solve(convertToEigen(matrixB))});
}

Matrixd SparseMatrix::solveCholesky(const Matrixd &matrixB) const {
  Eigen::SparseMatrix matrixA{convertToEigen(*this)};
  Eigen::SimplicialCholesky decompA{matrixA};
  if (decompA.info() != Eigen::Success) throw Error(std::runtime_error("Sparse Cholesky decomposition failed! ({})"_format(infoToString(decompA.info()))));
  return convertBackFromEigen(Eigen::MatrixXd{decompA.solve(convertToEigen(matrixB))});
}

std::pair<Vectorcd, Matrixcd> SparseMatrix::solveEigs(SortRule rule, int count) const {
  count = clamp(count, 1, int(rows()) - 2);
  Eigen::SparseMatrix matrix{convertToEigen(*this)};
  Spectra::SparseGenMatProd<double> matrixOp{matrix};
  Spectra::GenEigsSolver<decltype(matrixOp)> solver{matrixOp, count, clamp(int(count) * 3 / 2, 6, int(rows()))};
  solver.init();
  solver.compute(rule == SortRule::Largest ? Spectra::SortRule::LargestMagn : Spectra::SortRule::SmallestMagn);
  if (solver.info() == Spectra::CompInfo::Successful) return {convertBackFromEigen(solver.eigenvalues()), convertBackFromEigen(solver.eigenvectors())};
  throw Error(std::runtime_error("Generalized eigenvalue solver failed! ({})"_format(infoToString(solver.info()))));
  return {};
}

std::pair<Vectord, Matrixd> SparseMatrix::solveEigsCholesky(SortRule rule, int count) const {
  count = clamp(count, 1, int(rows()) - 2);
  Eigen::SparseMatrix matrix{convertToEigen(*this)};
  Spectra::SparseSymMatProd<double> matrixOp{matrix};
  Spectra::SymEigsSolver<decltype(matrixOp)> solver{matrixOp, count, clamp(count * 3 / 2, 6, int(rows()))};
  solver.init();
  solver.compute(rule == SortRule::Largest ? Spectra::SortRule::LargestAlge : Spectra::SortRule::SmallestAlge);
  if (solver.info() == Spectra::CompInfo::Successful) return {convertBackFromEigen(solver.eigenvalues()), convertBackFromEigen(solver.eigenvectors())};
  throw Error(std::runtime_error("Generalized eigenvalue solver failed! ({})"_format(infoToString(solver.info()))));
  return {};
}

std::pair<Vectord, Matrixd> SparseMatrix::solveEigsCholesky(SortRule rule, int count, const SparseMatrix &matrixI) const {
  count = clamp(count, 1, int(rows()) - 2);
  Eigen::SparseMatrix matrixA{convertToEigen(*this)};
  Eigen::SparseMatrix matrixB{convertToEigen(matrixI)};
  Spectra::SparseSymMatProd<double> matrixOpA{matrixA};
  Spectra::SparseCholesky<double> matrixOpB{matrixB};
  Spectra::SymGEigsSolver<decltype(matrixOpA), decltype(matrixOpB), Spectra::GEigsMode::Cholesky> solver{matrixOpA, matrixOpB, count, clamp(count * 3 / 2, 6, int(rows()))};
  solver.init();
  solver.compute(rule == SortRule::Largest ? Spectra::SortRule::LargestAlge : Spectra::SortRule::SmallestAlge);
  if (solver.info() == Spectra::CompInfo::Successful) return {convertBackFromEigen(solver.eigenvalues()), convertBackFromEigen(solver.eigenvectors())};
  throw Error(std::runtime_error("Generalized eigenvalue solver failed! ({})"_format(infoToString(solver.info()))));
  return {};
}

} // namespace mi::geometry
