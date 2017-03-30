#include "lwtnn/LightweightGraph.hh"
#include "lwtnn/InputPreprocessor.hh"
#include "lwtnn/Graph.hh"
#include <Eigen/Dense>

namespace {
  using namespace Eigen;
  using namespace lwt;

  // utility functions
  typedef LightweightGraph::NodeMap NodeMap;
  typedef InputPreprocessor IP;
  typedef std::vector<std::pair<std::string, IP*> > Preprocs;
  std::vector<VectorXd> get_input_vectors(const NodeMap& nodes,
                                          const Preprocs& preprocs) {
    std::vector<VectorXd> input_vectors;
    for (const auto& proc: preprocs) {
      if (!nodes.count(proc.first)) {
        throw NNEvaluationException("Can't find node " + proc.first);
      }
      const auto& preproc = *proc.second;
      input_vectors.emplace_back(preproc(nodes.at(proc.first)));
    }
    return input_vectors;
  }
  typedef LightweightGraph::SeqNodeMap SeqNodeMap;
  typedef InputVectorPreprocessor IVP;
  typedef std::vector<std::pair<std::string, IVP*> > VecPreprocs;
  std::vector<MatrixXd> get_input_seq(const SeqNodeMap& nodes,
                                      const VecPreprocs& preprocs) {
    std::vector<MatrixXd> input_mats;
    for (const auto& proc: preprocs) {
      if (!nodes.count(proc.first)) {
        throw NNEvaluationException("Can't find node " + proc.first);
      }
      const auto& preproc = *proc.second;
      input_mats.emplace_back(preproc(nodes.at(proc.first)));
    }
    return input_mats;
  }

}
namespace lwt {
  // ______________________________________________________________________
  // Lightweight Graph

  typedef LightweightGraph::NodeMap NodeMap;
  LightweightGraph::LightweightGraph(const GraphConfig& config,
                                     std::string default_output):
    m_graph(new Graph(config.nodes, config.layers))
  {
    for (const auto& node: config.inputs) {
      m_preprocs.emplace_back(
        node.name, new InputPreprocessor(node.variables));
    }
    for (const auto& node: config.input_sequences) {
      m_vec_preprocs.emplace_back(
        node.name, new InputVectorPreprocessor(node.variables));
    }
    size_t output_n = 0;
    for (const auto& node: config.outputs) {
      m_outputs.emplace_back(node.second.node_index, node.second.labels);
      m_output_indices.emplace(node.first, output_n);
      output_n++;
    }
    if (default_output.size() > 0) {
      if (!m_output_indices.count(default_output)) {
        throw NNConfigurationException("no output node" + default_output);
      }
      m_default_output = m_output_indices.at(default_output);
    } else if (output_n == 1) {
      m_default_output = 0;
    } else {
      throw NNConfigurationException("you must specify a default output");
    }
  }

  LightweightGraph::~LightweightGraph() {
    delete m_graph;
    for (auto& preproc: m_preprocs) {
      delete preproc.second;
      preproc.second = 0;
    }
    for (auto& preproc: m_vec_preprocs) {
      delete preproc.second;
      preproc.second = 0;
    }
  }

  ValueMap LightweightGraph::compute(const NodeMap& nodes,
                                     const SeqNodeMap& seq) const {
    return compute(nodes, seq, m_default_output);
  }
  ValueMap LightweightGraph::compute(const NodeMap& nodes,
                                     const SeqNodeMap& seq,
                                     const std::string& output) const {
    if (!m_output_indices.count(output)) {
      throw NNEvaluationException("no output node " + output);
    }
    return compute(nodes, seq, m_output_indices.at(output));
  }
  ValueMap LightweightGraph::compute(const NodeMap& nodes,
                                     const SeqNodeMap& seq,
                                     size_t idx) const {
    VectorSource source(get_input_vectors(nodes, m_preprocs),
                        get_input_seq(seq, m_vec_preprocs));
    VectorXd result = m_graph->compute(source, m_outputs.at(idx).first);
    const std::vector<std::string>& labels = m_outputs.at(idx).second;
    std::map<std::string, double> output;
    for (size_t iii = 0; iii < labels.size(); iii++) {
      output[labels.at(iii)] = result(iii);
    }
    return output;
  }

}
