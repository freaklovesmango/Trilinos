// @HEADER
// ***********************************************************************
//
//                Copyright message goes here.   TODO
//
// ***********************************************************************
// @HEADER

/*! \file Zoltan2_Model.hpp
    \brief Defines the Model interface.
*/

#ifndef _ZOLTAN2_MODEL_HPP_
#define _ZOLTAN2_MODEL_HPP_

#include <Zoltan2_Standards.hpp>
#include <Zoltan2_IdentifierMap.hpp>
#include <Zoltan2_StridedData.hpp>
#include <bitset>

namespace Zoltan2 {

/*! \brief An identifier for the general type of model.
 */
enum ModelType {
  InvalidModel = 0,
  HypergraphModelType,
  GraphModelType,
  CoordinateModelType,
  IdentifierModelType
} ;

/*! \brief Flags are set by a Problem to tell a Model what transformations
 *          it may need to do on the user's input.
 */ 
enum ModelFlags{
  // General flags
  IDS_MUST_BE_GLOBALLY_CONSECUTIVE, /*!< \brief algorithm requires consecutive ids */

  // Graph model flags
  SYMMETRIZE_INPUT_TRANSPOSE, /*!< \brief model must symmetrize input */ 
  SYMMETRIZE_INPUT_BIPARTITE, /*!< \brief model must symmetrize input */
  VERTICES_ARE_MATRIX_ROWS,   /*!< \brief use matrix rows as graph vertices */
  VERTICES_ARE_MATRIX_COLUMNS,/*!< \brief use columns as graph vertices */
  VERTICES_ARE_MATRIX_NONZEROS, /*!< \brief use nonzeros as graph vertices */
  VERTICES_ARE_MESH_NODES,    /*!< \brief use mesh nodes as vertices */
  VERTICES_ARE_MESH_ELEMENTS, /*!< \brief use mesh elements as vertices */
  SELF_EDGES_MUST_BE_REMOVED, /*!< \brief algorithm requires no self edges */
  GRAPH_IS_A_SUBSET_GRAPH,    /*!< \brief ignore invalid neighbors */

  NUM_MODEL_FLAGS
};

typedef std::bitset<NUM_MODEL_FLAGS> modelFlag_t;

/*! \brief The base class for all model classes.

  The Model is the computational model created by a Problem based on
  the user's input data and parameters.  Graphs, hypergraph, and 
  collections of geometric coordinates are examples of computational
  models.

  The Problem passes the Model to an algorithm.
  The algorithm queries the Model for input to its calculation.

  \todo Add HypergraphModel, CoordinateModel

*/

template <typename Adapter>
class Model
{
public:

#ifndef DOXYGEN_SHOULD_SKIP_THIS
  typedef typename Adapter::lno_t    lno_t;
  typedef typename Adapter::gno_t    gno_t;
  typedef typename Adapter::gid_t    gid_t;
  typedef typename Adapter::scalar_t    scalar_t;
  typedef typename Adapter::user_t    user_t;
  typedef StridedData<lno_t, scalar_t> input_t;
  typedef IdentifierMap<user_t> idmap_t;
#endif

  /*! Destructor
   */
  virtual ~Model() {};

  /*! Constructor
   */
  Model() : idMap_(), weightDim_(0), uniform_() {}

   /*! \brief Return the map from user global identifiers to internal
   *                Zoltan2 global numbers.
   *
   *  Every model must have an IdentifierMap, whether it needs for mapping 
   *  or not. The Map can simply indicate that Zoltan2 global numbers are 
   *  identical to the application's global IDs.
   */
  RCP<const idmap_t > getIdentifierMap() const { return idMap_; }

  /*! \brief Return the number of weights supplied for each object.
   *   If the user supplied no weights, dimension one is returned, because
   *   one dimension of uniform weights is implied.
   */
  int getNumWeights() const { return weightDim_;}

  /*! \brief Return whether the weights are uniform or not.
   *  \param weightDim a value from 0 to one less than the number of weights.
   *  \return 1 if the weights for that dimension are uniform, 0 if there
   *          is a list of differing weights for that dimension.
   */
  bool uniformWeight(int weightDim) const { return uniform_[weightDim];}

  /*!  \brief Return the local number of objects.
   *
   * Return the local number of objects, which may be
   *  vertices, matrix rows, identifiers, coordinates,
   *  or mesh nodes or elements.
   */
  virtual size_t getLocalNumObjects() const = 0;

  /*! \brief Return the global number of objects.
   *
   *  Return the global number of objects, which may be
   *  vertices, matrix rows, identifiers, coordinates,
   *  or mesh nodes or elements.
   */
  virtual global_size_t getGlobalNumObjects() const = 0;

  /*! \brief Get a list of the global Ids for the local objects.
   *
   *  Set a view to the list of object global numbers, which may be
   *  vertex IDs, matrix row IDs, identifiers, coordinate IDs,
   *  or mesh node or element IDs.
   */
  virtual void getGlobalObjectIds(ArrayView<const gno_t> &gnos) const = 0;

protected:

  /*! \brief Set the IdentifierMap used by the model.
   *
   *  The Model should set the identifier map with this call
   *  during the constructor.
   */
  void setIdentifierMap(RCP<const idmap_t> &map) { idMap_ = map; }

  /*! \brief Set the length of each weight array.  
   * The Model calls this in the constructor so we know which
   * weights are uniform.  If lengths for a given weight dimension
   * are zero on all processes, then we know that uniform weights are implied.
   *
   * This must be called by all processes.
   */
  void setWeightArrayLengths(const Array<lno_t> &len, 
    const Teuchos::Comm<int> &comm)
  {
    weightDim_ = len.size();

    if (weightDim_ < 1)
      weightDim_ = 1;          // uniform weights are implied

    int *lval = new int [weightDim_];
    uniform_ = arcp(lval, 0, weightDim_);

    if (len.size() < 1){
      uniform_[0] = 1;
      return;
    }

    for (int i=0; i < weightDim_; i++){
      if (len[i] > 0)
        lval[i] = 1;
      else
        lval[i] = 0;
    }

    int *rval = new int [weightDim_];

    try{
      reduceAll<int, int>(comm, Teuchos::REDUCE_MAX, weightDim_, lval, rval);
    }
    Z2_FORWARD_EXCEPTIONS

    for (int i=0; i < weightDim_; i++){
      if (rval[i] > 0)
        uniform_[i] = 0;
      else
        uniform_[i] = 1;
    }
 
    delete [] rval;
  }

private:

  RCP<const idmap_t> idMap_;

  int weightDim_;       /*!< Minimum of 1 or number of user-supplied weights */
  ArrayRCP<int> uniform_;   /*!< weightDim_ flags, 1 if uniform, 0 if not.   */
};

}   //  namespace Zoltan2

#endif
