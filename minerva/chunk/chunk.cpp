#include <functional>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <glog/logging.h>
#include "chunk.h"
#include "dag/physical_dag.h"
#include "op/physical_op.h"
#include "op/shared_op.h"
#include "system/minerva_system.h"
#include "system/data_store.h"

using namespace std;

namespace minerva {

Chunk::Chunk(): data_node_(NULL) { }
Chunk::Chunk(PhysicalDataNode* node): data_node_(node) { }
Chunk::Chunk(const Chunk& other): data_node_(other.data_node_) { }
Chunk& Chunk::operator = (const Chunk& other) {
  data_node_ = other.data_node_;
  return *this;
}

/////////////////////////////////////////////////////////
// General interface for customized operations
/////////////////////////////////////////////////////////
// TODO deprecated
std::vector<Chunk> Chunk::Compute(const std::vector<Chunk>& params,
      const std::vector<Scale>& result_sizes, PhysicalComputeFn* fn) {
  auto& ms = MinervaSystem::Instance();
  auto& pdag = ms.physical_dag();
  vector<Chunk> rst;
  vector<PhysicalDataNode*> rst_data_nodes;
  for (auto& size: result_sizes) {
    PhysicalData phy_data;
    // TODO how to set place ?
    phy_data.size = size;
    phy_data.data_id = ms.data_store().GenerateDataID(); // generate data id
    phy_data.device_info = fn->device_info;
    auto rst_node = pdag.NewDataNode(phy_data);
    rst.push_back(Chunk(rst_node));
    rst_data_nodes.push_back(rst_node);
  }
  vector<PhysicalDataNode*> param_data_nodes;
  for (auto& ch: params) {
    param_data_nodes.push_back(ch.data_node());
  }
  PhysicalOp phy_op;
  // TODO how to set place ?
  phy_op.impl_type = ImplType::kNA;
  phy_op.compute_fn = fn;
  pdag.NewOpNode(param_data_nodes, rst_data_nodes, phy_op);
  return rst;
}

/* TODO
vector<Chunk> Chunk::Compute(
      const std::vector<Chunk>& params,
      const std::vector<Scale>& result_sizes,
      PhysicalComputeFn* fn,
      DeviceInfo info) {
  auto& ms = MinervaSystem::Instance();
  auto& pdag = ms.physical_dag();
  vector<Chunk> rst(result_sizes.size());
  transform(result_sizes.begin(), result_sizes.end(), rst.begin(), [&](const Scale& size) {
    PhysicalData phy_data;
    phy_data.size = size;
    phy_data.data_id = ms.data_store().GenerateDataID();
    phy_data.device_info = info;
    auto rst_node = pdag.NewDataNode(phy_data);
    return Chunk(rst_node);
  });
  vector<PhysicalDataNode*> rst_data_nodes(rst.size());
  transform(rst.begin(), rst.end(), rst_data_nodes.begin(), [](const Chunk& i) {
    return i.data_node();
  });
  vector<PhysicalDataNode*> param_data_nodes(params.size());
  transform(params.begin(), params.end(), param_data_nodes.begin(), [](const Chunk& i) {
    return i.data_node();
  });
  PhysicalOp phy_op;
  phy_op.impl_type = ImplType::kNA;
  phy_op.compute_fn = fn;
  phy_op.device_info = info;
  pdag.NewOpNode(param_data_nodes, rst_data_nodes, phy_op);
  return rst;
}
*/

/////////////////////////////////////////////////////////
// Data generator
/////////////////////////////////////////////////////////
// TODO deprecated
Chunk Chunk::Constant(const Scale& size, float val) {
  FillOp* fill_op = new FillOp;
  fill_op->closure = {val};
  return Chunk::Compute({}, {size}, fill_op)[0];
}

// TODO deprecated
Chunk Chunk::Randn(const Scale& size, float mu, float var) {
  RandnOp* randn_op = new RandnOp;
  randn_op->closure = {mu, var};
  return Chunk::Compute({}, {size}, randn_op)[0];
}

Scale Chunk::ComputeOffset(NVector<Chunk> chunks) {
  const Scale& numparts = chunks.Size();
  size_t numdims = numparts.NumDims();
  Scale pos = Scale::Origin(numdims);
  while(1) {
    auto& phy_data = chunks[pos].data_node()->data_;
    phy_data.offset.Resize(numdims, 0);
    phy_data.offset_index = pos;
    Scale upleftpos = pos.Map([] (int x) { return max(x - 1, 0); });
    auto& upleft_phy_data = chunks[upleftpos].data_node()->data_;
    for(size_t i = 0; i < numdims; ++i) {
      if(pos[i] != 0) { // if the index of this dimension is 0, the offset should be 0 too
        phy_data.offset[i] = upleft_phy_data.offset[i] + upleft_phy_data.size[i];
      }
    }
    if(!Scale::IncrOne(pos, numparts)) {
      return phy_data.offset + phy_data.size;
    }
  }
}

Chunk Chunk::Merge(NVector<Chunk> parts) {
  Scale rstsize = ComputeOffset(parts);
  AssembleOp* assemble_op = new AssembleOp;
  return Chunk::Compute(parts.ToVector(), {rstsize}, assemble_op)[0];
}

NVector<Chunk> Chunk::Split(const NVector<Scale>& partsizes) {
  SplitOp* split_op = new SplitOp;
  const auto& rstchunklst = Compute({*this}, partsizes.ToVector(), split_op);
  NVector<Chunk> rstchunknvec(rstchunklst, partsizes.Size());
  ComputeOffset(rstchunknvec);
  return rstchunknvec;
}

} // end of namespace minerva
