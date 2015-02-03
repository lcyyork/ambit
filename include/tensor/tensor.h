#if !defined(TENSOR_INCLUDE_TENSOR_H)
#define TENSOR_INCLUDE_TENSOR_H

#include <cstdio>
#include <utility>
#include <vector>
#include <map>
#include <string>

#if defined(CXX11)
#include <memory>
#include <tuple>

namespace tensor {

    /*
     * If we have C++11 then we don't need Boost for shared_ptr, tuple, and unique_ptr.
     */
    using std::tuple;
    using std::shared_ptr;
    using std::unique_ptr;

}

#else
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/tuple/tuple.hpp>

namespace tensor {

    using boost::tuple;
    using boost::make_tuple;
    using boost::shared_ptr;
    template<class T> using unique_ptr = boost::scoped_ptr<T>;

}

#endif

namespace tensor {

static constexpr double numerical_zero__ = 1.0e-15;

class TensorImpl;
class LabeledTensor;
class LabeledTensorProduct;
class LabeledTensorAddition;
class LabeledTensorSubtraction;
class LabeledTensorDistributive;
class LabeledTensorSumOfProducts;
class SlicedTensor;

enum TensorType {
    kCurrent, kCore, kDisk, kDistributed, kAgnostic
};
enum EigenvalueOrder {
    kAscending, kDescending
};

typedef std::vector<size_t> Dimension;
typedef std::vector<std::vector<size_t>> IndexRange;
typedef std::vector<std::string> Indices;

/** Initializes the tensor library.
 *
 * Calls any necessary initialization of utilized frameworks.
 * @param argc number of command line arguments
 * @param argv the command line arguments
 * @return error code
 */
int initialize(int argc, char** argv);

/** Shutdowns the tensor library.
 *
 * Calls any necessary routines of utilized frameworks.
 */
void finalize();

/**
 * Class Tensor is 
 **/
class Tensor {

public:

    // => Constructors <= //

    static Tensor build(TensorType type, const std::string& name, const Dimension& dims);

    static Tensor build(TensorType type, const Tensor& other);

    void copy(const Tensor& other, const double& scale = 1.0);

    Tensor();

    // => Reflectors <= //

    TensorType type() const;
    std::string name() const;
    const Dimension& dims() const;
    size_t dim(size_t index) const;
    size_t rank() const;
    /// \return Total number of elements in the tensor.
    size_t numel() const;

    /**
     * Print some tensor information to fh
     * \param level If level = false, just print name and dimensions.  If level = true, print the entire tensor.
     **/
    void print(FILE* fh, bool level = false, const std::string& format = std::string("%11.6f"), int maxcols = 5) const;

    // => Labelers <= //

    LabeledTensor operator()(const std::string& indices);
    LabeledTensor operator[](const std::string& indices);

    // => Slicers <= //

    SlicedTensor operator()(const IndexRange& indices);
    SlicedTensor operator[](const IndexRange& indices);

    // => Setters/Getters <= //

    /**
     * Returns the raw data vector underlying the tensor object
     * if the underlying tensor object supports a raw data vector.
     * This is only the case if the underlying tensor is of type kCore.
     *
     * This routine is intended to facilitate rapid filling of data into a
     * kCore buffer tensor, following which the user may stripe the buffer
     * tensor into a kDisk or kDistributed tensor via slice operations. 
     *
     * If a vector is successfully returned, it points to the unrolled
     * data of the tensor, with the right-most dimensions running fastest
     * and left-most dimensions running slowest.
     *
     * Example successful use case:
     *  Tensor A = Tensor::build(kCore, "A3", {4,5,6});
     *  std::vector<double>& Av = A.data();
     *  double* Ap = Av.data(); // In case the raw pointer is needed
     *  In this case, Av[0] = A(0,0,0), Av[1] = A(0,0,1), etc.
     *
     * Example unsuccessful use case:
     *  Tensor B = Tensor::build(kDisk, "B3", {4,5,6});
     *  std::vector<double>& Bv = B.data(); // throws
     *
     * Results:
     *  @return data pointer, if tensor object supports it
     **/
    std::vector<double>& data();
    const std::vector<double>& data() const;

    // => Slicers <= //

//    static Tensor slice(const Tensor& tensor, const IndexRange& ranges);
    static Tensor cat(const std::vector<Tensor>, int dim);

    // => Simple Single Tensor Operations <= //

    Tensor& zero();
    Tensor& scale(double a);
    double norm(double power = 2.0) const;

    // => Simple Double Tensor Operations <= //

    /**
    * Performs: C["ij"] += 2.0 * A["ij"];
    */
    Tensor& scale_and_add(const double& a, const Tensor& x);
    /**
    * Performs: C["ij"] *= A["ij"];
     */
    Tensor& pointwise_multiplication(const Tensor& x);
    /**
    * Performs: C["ij"] /= A["ij"];
    */
    Tensor& pointwise_division(const Tensor& x);
    double dot(const Tensor& x);

    // => Order-2 Operations <= //

    std::map<std::string, Tensor> syev(EigenvalueOrder order);
    std::map<std::string, Tensor> geev(EigenvalueOrder order);
    std::map<std::string, Tensor> svd();

    Tensor cholesky();
    std::map<std::string, Tensor> lu();
    std::map<std::string, Tensor> qr();

    Tensor cholesky_inverse();
    Tensor inverse();
    Tensor power(double power, double condition = 1.0E-12);

    // => Contraction Type Operations <= //

    /**
     * Perform the contraction:
     *  C(Cinds) = alpha * A(Ainds) * B(Binds) + beta * C(Cinds)
     *   
     * Note: Most users should instead use the operator overloading
     * routines, e.g.,
     *  C2("ij") += 0.5 * A2("ik") * B2("jk");
     *
     * Parameters:
     *  @param A: The left-side factor tensor, e.g., A2
     *  @param B: The right-side factor tensor, e.g., B2
     *  @param Cinds: The indices of tensor C, e.g., "ij"
     *  @param Ainds: The indices of tensor A, e.g., "ik"
     *  @param Binds: The indices of tensor B, e.g., "jk"
     *  @param alpha: The scale applied to the product A*B, e.g., 0.5
     *  @param beta: The scale applied to the tensor C, e.g., 1.0
     *
     * Results:
     *  @return void
     *  C is the current tensor, whose data is overwritten. e.g., C2
     **/
    void contract(
        const Tensor& A,
        const Tensor& B,
        const std::vector<std::string>& Cinds,
        const std::vector<std::string>& Ainds,
        const std::vector<std::string>& Binds,
        double alpha = 1.0,
        double beta = 0.0);

    /**
     * Perform the permutation:
     *  C(Cinds) = alpha * A(Ainds) + beta * C(Cinds)
     *   
     * Note: Most users should instead use the operator overloading
     * routines, e.g.,
     *  C2("ij") += 0.5 * A2("ji");
     *
     * Parameters:
     *  @param A: The source tensor, e.g., A2
     *  @param Cinds: The indices of tensor C, e.g., "ij"
     *  @param Ainds: The indices of tensor A, e.g., "ji"
     *  @param alpha: The scale applied to the tensor A, e.g., 0.5
     *  @param beta: The scale applied to the tensor C, e.g., 1.0
     *
     * Results:
     *  @return void
     *  C is the current tensor, whose data is overwritten. e.g., C2
     **/
    void permute(
        const Tensor& A,
        const std::vector<std::string>& Cinds,
        const std::vector<std::string>& Ainds,
        double alpha = 1.0,
        double beta = 0.0);

    /**
     * Perform the slice:
     *  C(Cinds) = alpha * A(Ainds) + beta * C(Cinds)
     *   
     * Note: Most users should instead use the operator overloading
     * routines, e.g.,
     *  C2({{0,m},{0,n}}) += 0.5 * A2({{1,m+1},{1,n+1}});
     *  TODO: This must be coded. Possibly IndexRange should be changed to vector<vector<size_t>> for brevity
     *
     * Parameters:
     *  @param A: The source tensor, e.g., A2
     *  @param Cinds: The slices of indices of tensor C, e.g., {{0,m},{0,n}}
     *  @param Ainds: The indices of tensor A, e.g., {{1,m+1},{1,n+1}}
     *  @param alpha: The scale applied to the tensor A, e.g., 0.5
     *  @param beta: The scale applied to the tensor C, e.g., 1.0
     *
     * Results:
     *  @return void
     *  C is the current tensor, whose data is overwritten. e.g., C2
     **/
    void slice(
        const Tensor& A,
        const IndexRange& Cinds,
        const IndexRange& Ainds,
        double alpha = 1.0,
        double beta = 0.0);

    bool operator==(const Tensor& other) const;
    bool operator!=(const Tensor& other) const;

private:

    shared_ptr<TensorImpl> tensor_;

protected:

    Tensor(shared_ptr<TensorImpl> tensor);

    std::map<std::string, Tensor> map_to_tensor(const std::map<std::string, TensorImpl*>& x);
};

class LabeledTensor {

public:
    LabeledTensor(Tensor T, const std::vector<std::string>& indices, double factor = 1.0);

    double factor() const { return factor_; }
    const Indices& indices() const { return indices_; }
    Tensor T() const { return T_; }

    LabeledTensorProduct operator*(const LabeledTensor& rhs);
    LabeledTensorAddition operator+(const LabeledTensor& rhs);
    LabeledTensorAddition operator-(const LabeledTensor& rhs);

    LabeledTensorDistributive operator*(const LabeledTensorAddition& rhs);

    /** Copies data from rhs to this sorting the data if needed. */
    void operator=(const LabeledTensor& rhs);
    void operator+=(const LabeledTensor& rhs);
    void operator-=(const LabeledTensor& rhs);
    void operator=(const LabeledTensorDistributive& rhs);
    void operator+=(const LabeledTensorDistributive& rhs);
    void operator-=(const LabeledTensorDistributive& rhs);

    void operator=(const LabeledTensorProduct& rhs);
    void operator+=(const LabeledTensorProduct& rhs);
    void operator-=(const LabeledTensorProduct& rhs);

    void operator=(const LabeledTensorAddition& rhs);
    void operator+=(const LabeledTensorAddition& rhs);
    void operator-=(const LabeledTensorAddition& rhs);

    void operator*=(const double& scale);
    void operator/=(const double& scale);

//    bool operator==(const LabeledTensor& other) const;
//    bool operator!=(const LabeledTensor& other) const;

    size_t numdim() const { return indices_.size(); }
    size_t dim_by_index(const std::string& idx) const;

    // negation
    LabeledTensor operator-() const {
        return LabeledTensor(T_, indices_, -factor_);
    }

private:

    void set(const LabeledTensor& to);

    Tensor T_;
    std::vector<std::string> indices_;
    double factor_;

};

inline LabeledTensor operator*(double factor, const LabeledTensor& ti) {
    return LabeledTensor(ti.T(), ti.indices(), factor*ti.factor());
};

class LabeledTensorProduct {

public:
    LabeledTensorProduct(const LabeledTensor& A, const LabeledTensor& B)
    {
        tensors_.push_back(A);
        tensors_.push_back(B);
    }

    size_t size() const { return tensors_.size(); }

    const LabeledTensor& operator[](size_t i) const { return tensors_[i]; }

    LabeledTensorProduct& operator*(const LabeledTensor& other) {
        tensors_.push_back(other);
        return *this;
    }

    // conversion operator
    operator double() const;

    std::pair<double, double> compute_contraction_cost(const std::vector<size_t>& perm) const;

private:

    std::vector<LabeledTensor> tensors_;
};

class LabeledTensorAddition
{
public:
    LabeledTensorAddition(const LabeledTensor& A, const LabeledTensor& B)
    {
        tensors_.push_back(A);
        tensors_.push_back(B);
    }

    size_t size() const { return tensors_.size(); }

    const LabeledTensor& operator[](size_t i) const { return tensors_[i]; }

    std::vector<LabeledTensor>::iterator begin() { return tensors_.begin(); }
    std::vector<LabeledTensor>::const_iterator begin() const { return tensors_.begin(); }

    std::vector<LabeledTensor>::iterator end() { return tensors_.end(); }
    std::vector<LabeledTensor>::const_iterator end() const { return tensors_.end(); }

    LabeledTensorAddition& operator+(const LabeledTensor& other) {
        tensors_.push_back(other);
        return *this;
    }

    LabeledTensorAddition& operator-(const LabeledTensor& other) {
        tensors_.push_back(-other);
        return *this;
    }

    LabeledTensorDistributive operator*(const LabeledTensor& other);

    LabeledTensorAddition& operator*(const double& scalar);

    // negation
    LabeledTensorAddition& operator-();

private:

    // This handles cases like T("ijab")
    std::vector<LabeledTensor> tensors_;

};

inline LabeledTensorAddition operator*(double factor, const LabeledTensorAddition& ti) {
    LabeledTensorAddition ti2 = ti;
    return ti2 * factor;
}

// Is responsible for expressions like D * (J - K) --> D*J - D*K
class LabeledTensorDistributive
{
public:
    LabeledTensorDistributive(const LabeledTensor& A, const LabeledTensorAddition& B)
            : A_(A), B_(B)
    {}

    const LabeledTensor& A() const { return A_; }
    const LabeledTensorAddition& B() const { return B_; }

    // conversion operator
    operator double() const;

private:

    const LabeledTensor& A_;
    const LabeledTensorAddition& B_;

};

class SlicedTensor
{
public:
    SlicedTensor(Tensor T, const IndexRange& range, double factor = 1.0);

    double factor() const { return factor_; }
    const IndexRange& range() const { return range_; }
    Tensor T() const { return T_; }

    void operator=(const SlicedTensor& rhs);
    void operator+=(const SlicedTensor& rhs);
    void operator-=(const SlicedTensor& rhs);

private:
    Tensor T_;
    IndexRange range_;
    double factor_;
};

inline SlicedTensor operator*(double factor, const SlicedTensor& ti) {
    return SlicedTensor(ti.T(), ti.range(), factor*ti.factor());
};


}

#endif

