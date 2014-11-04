// Copyright 2014 Project Athena

#include <boost/python.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/args.hpp>

#include <iostream>
using namespace std;

#include "minerva.h"

namespace bp = boost::python;
namespace m = minerva;

namespace owl {

void Initialize(bp::list args) {
  int argc = bp::extract<int>(args.attr("__len__")());
  char** argv = new char*[argc];
  for (int i = 0; i < argc; i++) {
    argv[i] = bp::extract<char*>(args[i]);
  }
  m::MinervaSystem::Instance().Initialize(&argc, &argv);
}

uint64_t CreateCpuDevice() {
  return m::MinervaSystem::Instance().device_manager().CreateCpuDevice();
}

uint64_t CreateGpuDevice(int id) {
  return m::MinervaSystem::Instance().device_manager().CreateGpuDevice(id);
}

void SetDevice(uint64_t id) {
  m::MinervaSystem::Instance().current_device_id_ = id;
}

m::NArray Softmax(m::NArray m) {
  m::NArray maxval = m.Max(0);
  // NArray centered = m - maxval.Tile({m.Size(0), 1});
  m::NArray centered = m.NormArithmetic(maxval, m::ArithmeticType::kSub);
  m::NArray class_normalizer = m::Elewise::Ln(m::Elewise::Exp(centered).Sum(0)) + maxval;
  // return Elewise::Exp(m - class_normalizer.Tile({m.Size(0), 1}));
  return m::Elewise::Exp(m.NormArithmetic(class_normalizer, m::ArithmeticType::kSub));
}

m::Scale ToScale(const bp::list& l) {
  bp::stl_input_iterator<int> begin(l), end;
  return m::Scale(begin, end);
}

bp::list ToPythonList(const m::Scale& s) {
  bp::list l;
  for(int i : s) {
    l.append(i);
  }
  return l;
}

m::NArray ZerosWrapper(const bp::list& s) {
  return m::NArray::Zeros(ToScale(s));
}

m::NArray OnesWrapper(const bp::list& s) {
  return m::NArray::Ones(ToScale(s));
}

m::NArray RandnWrapper(const bp::list& s, float mean, float var) {
  return m::NArray::Randn(ToScale(s), mean, var);
}

m::NArray MakeNArrayWrapper(const bp::list& s, const bp::list& val) {
  std::vector<float> v = std::vector<float>(bp::stl_input_iterator<float>(val), bp::stl_input_iterator<float>());
  size_t length = bp::len(val);
  shared_ptr<float> data( new float[length], [] (float* p) { delete [] p; } );
  memcpy(data.get(), v.data(), sizeof(float) * length);
//  for(size_t i = 0; i < length; ++i) {
//    valptr.get()[i] = bp::extract<float>(val[i] * 1.0);
//  }
  return m::NArray::MakeNArray(ToScale(s), data);
}
bp::list NArrayToList(m::NArray narr) {
  bp::list l;
  std::shared_ptr<float> v = narr.Get();
  for(int i = 0; i < narr.Size().Prod(); ++i)
    l.append(v.get()[i]);
  return l;
}
bp::list NArrayGetShapeWrapper(m::NArray narr) {
  return ToPythonList(narr.Size());
}

void WaitForEvalFinish() {
  m::MinervaSystem::Instance().WaitForEvalFinish();
}

void PrintSystemInfo() {
  m::MinervaSystem& ms = m::MinervaSystem::Instance();
  cout << "CPU mem:" << ms.device_manager().GetDevice(0)->GetMemUsage() << endl;
  cout << "GPU mem:" << ms.device_manager().GetDevice(1)->GetMemUsage() << endl;
  cout << "Dag:\n" << ms.physical_dag().PrintDag<m::ExternRCPrinter>() << endl;
}

} // end of namespace owl

// python module
BOOST_PYTHON_MODULE(libowl) {
  using namespace boost::python;

  enum_<m::ArithmeticType>("arithmetic")
    .value("add", m::ArithmeticType::kAdd)
    .value("sub", m::ArithmeticType::kSub)
    .value("mul", m::ArithmeticType::kMult)
    .value("div", m::ArithmeticType::kDiv)
  ;

  float (m::NArray::*sum0)() const = &m::NArray::Sum;
  m::NArray (m::NArray::*sum1)(int) const = &m::NArray::Sum;
  m::NArray (m::NArray::*sum2)(const m::Scale&) const = &m::NArray::Sum;

  float (m::NArray::*max0)() const = &m::NArray::Max;
  m::NArray (m::NArray::*max1)(int) const = &m::NArray::Max;
  m::NArray (m::NArray::*max2)(const m::Scale&) const = &m::NArray::Max;

  //class_<m::Scale>("Scale");
  class_<m::NArray>("NArray")
    // element-wise
    .def(self + self)
    .def(self - self)
    .def(self / self)
    .def(float() + self)
    .def(float() - self)
    .def(float() * self)
    .def(float() / self)
    .def(self + float())
    .def(self - float())
    .def(self * float())
    .def(self / float())
    // matrix multiply
    .def(self * self)
    // reduction
    .def("sum", sum0)
    .def("sum", sum1)
    .def("sum", sum2)
    .def("max", max0)
    .def("max", max1)
    .def("max", max2)
    .def("max_index", &m::NArray::MaxIndex)
    .def("count_zero", &m::NArray::CountZero)
    // normalize
    .def("norm_arithmetic", &m::NArray::NormArithmetic)
    // misc
    .def("shape", &owl::NArrayGetShapeWrapper)
    .def("trans", &m::NArray::Trans)
    .def("tofile", &m::NArray::ToFile)
    .def("tolist", &owl::NArrayToList)
    .def("eval", &m::NArray::Eval)
    .def("eval_async", &m::NArray::EvalAsync)
  ;
/*
  // file loader
  class_<m::IFileLoader>("IFileLoader");
  class_<m::SimpleFileLoader, bases<m::IFileLoader>>("SimpleFileLoader");
  class_<m::FileFormat>("FileFormat")
    .def_readwrite("binary", &m::FileFormat::binary)
  ;
*/
  // creators
  def("zeros", &owl::ZerosWrapper);
  def("ones", &owl::OnesWrapper);
  def("make_narray", &owl::MakeNArrayWrapper);
  def("randn", &owl::RandnWrapper);

  // system
  def("initialize", &owl::Initialize);
  def("create_cpu_device", &owl::CreateCpuDevice);
  def("create_gpu_device", &owl::CreateGpuDevice);
  def("set_device", &owl::SetDevice);
  def("wait_eval", &owl::WaitForEvalFinish);
  def("print_sys_info", &owl::PrintSystemInfo);

  // elewise
  def("mult", &m::Elewise::Mult);
  def("sigmoid", &m::Elewise::Sigmoid);
  def("exp", &m::Elewise::Exp);
  def("ln", &m::Elewise::Ln);
  
  // utils
  def("softmax", &owl::Softmax);
}
