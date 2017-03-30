#ifndef GRAPH_HH
#define GRAPH_HH

#include "NNLayerConfig.hh"

#include <Eigen/Dense>

#include <vector>

namespace lwt {

  class Stack;
  class RecurrentStack;

  using Eigen::VectorXd;
  using Eigen::MatrixXd;

  // this is called by input nodes to get the inputs
  class ISource
  {
  public:
    virtual VectorXd at(size_t index) const = 0;
    virtual MatrixXd matrix_at(size_t index) const = 0;
  };

  class VectorSource: public ISource
  {
  public:
    VectorSource(std::vector<VectorXd>&&, std::vector<MatrixXd>&& = {});
    virtual VectorXd at(size_t index) const;
    virtual MatrixXd matrix_at(size_t index) const;
  private:
    std::vector<VectorXd> m_inputs;
    std::vector<MatrixXd> m_matrix_inputs;
  };

  class DummySource: public ISource
  {
  public:
    DummySource(const std::vector<size_t>& input_sizes,
                const std::vector<std::pair<size_t, size_t> >& = {});
    virtual VectorXd at(size_t index) const;
    virtual MatrixXd matrix_at(size_t index) const;
  private:
    std::vector<size_t> m_sizes;
    std::vector<std::pair<size_t, size_t> > m_matrix_sizes;
  };


  // node class: will return a VectorXd from ISource
  class INode
  {
  public:
    virtual ~INode() {}
    virtual VectorXd compute(const ISource&) const = 0;
    virtual size_t n_outputs() const = 0;
  };

  class InputNode: public INode
  {
  public:
    InputNode(size_t index, size_t n_outputs);
    virtual VectorXd compute(const ISource&) const;
    virtual size_t n_outputs() const;
  private:
    size_t m_index;
    size_t m_n_outputs;
  };

  class FeedForwardNode: public INode
  {
  public:
    FeedForwardNode(const Stack*, const INode* source);
    virtual VectorXd compute(const ISource&) const;
    virtual size_t n_outputs() const;
  private:
    const Stack* m_stack;
    const INode* m_source;
  };

  class ConcatenateNode: public INode
  {
  public:
    ConcatenateNode(const std::vector<const INode*>&);
    virtual VectorXd compute(const ISource&) const;
    virtual size_t n_outputs() const;
  private:
    std::vector<const INode*> m_sources;
    size_t m_n_outputs;
  };

  // sequence nodes
  class ISequenceNode
  {
  public:
    virtual ~ISequenceNode() {}
    virtual MatrixXd scan(const ISource&) const = 0;
    virtual size_t n_outputs() const = 0;
  };

  class InputSequenceNode: public ISequenceNode
  {
  public:
    InputSequenceNode(size_t index, size_t n_outputs);
    virtual MatrixXd scan(const ISource&) const;
    virtual size_t n_outputs() const;
  private:
    size_t m_index;
    size_t m_n_outputs;
  };

  class SequenceNode: public ISequenceNode, public INode
  {
  public:
    SequenceNode(const RecurrentStack*, const ISequenceNode* source);
    virtual MatrixXd scan(const ISource&) const;
    virtual VectorXd compute(const ISource&) const;
    virtual size_t n_outputs() const;
  private:
    const RecurrentStack* m_stack;
    const ISequenceNode* m_source;
  };

  // Graph class, owns the nodes
  class Graph
  {
  public:
    Graph();                    // dummy constructor
    Graph(const std::vector<NodeConfig>& nodes,
          const std::vector<LayerConfig>& layers);
    Graph(Graph&) = delete;
    Graph& operator=(Graph&) = delete;
    ~Graph();
    VectorXd compute(const ISource&, size_t node_number) const;
    VectorXd compute(const ISource&) const;
  private:
    std::vector<INode*> m_nodes;
    std::vector<Stack*> m_stacks;
    std::vector<ISequenceNode*> m_seq_nodes;
    std::vector<RecurrentStack*> m_seq_stacks;
    // At some point maybe also convolutional nodes, but we'd have to
    // have a use case for that first.
  };
}

#endif // GRAPH_HH
