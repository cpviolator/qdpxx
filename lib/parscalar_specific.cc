// $Id: parscalar_specific.cc,v 1.5 2002-12-16 06:13:49 edwards Exp $
//
// QDP data parallel interface
//
// Layout
//
// This routine provides various layouts, including
//    lexicographic
//    2-checkerboard  (even/odd-checkerboarding of sites)
//    32-style checkerboard (even/odd-checkerboarding of hypercubes)

#include "qdp.h"
#include "proto.h"

#include "QMP.h"

#define  USE_LEXICO_LAYOUT
#undef   USE_CB2_LAYOUT
#undef   USE_CB32_LAYOUT

QDP_BEGIN_NAMESPACE(QDP);

//! Definition of shift function object
NearestNeighborMap  shift;


//-----------------------------------------------------------------------------
//! Private flag for status
static bool isInit = false;

//! Turn on the machine
void QDP_initialize(int *argc, char ***argv)
{
  if (isInit)
    QDP_error_exit("QDP already inited");

  QMP_verbose (QMP_TRUE);

  if (QMP_init_msg_passing(argc, argv, QMP_SMP_ONE_ADDRESS) != QMP_SUCCESS)
    QDP_error_exit("QDP_initialize failed");

  isInit = true;
}

//! Is the machine initialized?
bool QDP_isInitialized() {return isInit;}

//! Turn off the machine
void QDP_finalize()
{
  if ( ! QDP_isInitialized() )
    QDP_error_exit("QDP is not inited");

  QMP_finalize_msg_passing();
  isInit = false;
}

//! Panic button
void QDP_abort(int status)
{
  QDP_finalize(); 
  exit(status);
}



//-----------------------------------------------------------------------------

namespace Layout
{
  //-----------------------------------------------------
  //! Local data specific to all architectures
  /*! 
   * NOTE: the disadvantage to using a struct to keep things together is
   * that subsequent groupings of namespaces can not just add onto the
   * current namespace. This would be useful if say in a cb=2 implementation
   * the cb_nrow stuff is needed, but does not need to be there for the 
   * lexicographic implementation
   */
  struct
  {
    //! Total lattice volume
    int vol;

    //! Lattice size
    multi1d<int> nrow;

    //! Number of checkboards
    int nsubl;

    //! Total lattice checkerboarded volume
    int vol_cb;

    //! Checkboard lattice size
    multi1d<int> cb_nrow;

    //! Subgrid lattice volume
    int subgrid_vol;

    //! Subgrid lattice size
    multi1d<int> subgrid_nrow;

    //! Logical node coordinates
    multi1d<int> logical_coord;

    //! Logical system size
    multi1d<int> logical_size;

    //! Node rank
    int node_rank;

    //! Total number of nodes
    int num_nodes;
  } _layout;


  //-----------------------------------------------------
  // Functions

  //! Virtual grid (problem grid) lattice size
  const multi1d<int>& lattSize() {return _layout.nrow;}

  //! Total lattice volume
  int vol() {return _layout.vol;}

  //! Subgrid lattice volume
  int subgridVol() {return _layout.subgrid_vol;}

  //! Returns whether this is the primary node
  bool primaryNode() {return (_layout.node_rank == 0) ? true : false;}

  //! Subgrid (grid on each node) lattice size
  const multi1d<int>& subgridLattSize() {return _layout.subgrid_nrow;}

  //! Returns the node number of this node
  int nodeNumber() {return _layout.node_rank;}

  //! Returns the number of nodes
  int numNodes() {return _layout.num_nodes;}

  //! Returns the logical node coordinates for this node
  const multi1d<int>& nodeCoord() {return _layout.logical_coord;}

  //! Returns the logical size of this machine
  const multi1d<int>& logicalSize() {return _layout.logical_size;}




  //! The linearized site index for the corresponding lexicographic site
  int linearSiteIndex(int site)
  { 
    multi1d<int> coord = crtesn(site, lattSize());
    
    return linearSiteIndex(coord);
  }


  //! The lexicographic site index for the corresponding coordinate
  int lexicoSiteIndex(const multi1d<int>& coord)
  {
    return local_site(coord, lattSize());
  }

};


//-----------------------------------------------------------------------------
#if defined(USE_LEXICO_LAYOUT)

#warning "Using a lexicographic layout"

namespace Layout
{
  //! The linearized site index for the corresponding coordinate
  /*! This layout is a simple lexicographic lattice ordering */
  int linearSiteIndex(const multi1d<int>& coord)
  {
    multi1d<int> tmp_coord(Nd);

    for(int i=0; i < coord.size(); ++i)
      tmp_coord[i] = coord[i] % Layout::subgridLattSize()[i];
    
    return local_site(tmp_coord, Layout::subgridLattSize());
  }


  //! The lexicographic site index from the corresponding linearized site
  /*! This layout is a simple lexicographic lattice ordering */
  int lexicoSiteIndex(int linearsite)
  {
    return linearsite;
  }


  //! The node number for the corresponding lattice coordinate
  /*! This layout is a simple lexicographic lattice ordering */
  int nodeNumber(const multi1d<int>& coord)
  {
    multi1d<int> tmp_coord(Nd);

    for(int i=0; i < coord.size(); ++i)
      tmp_coord[i] = coord[i] / Layout::subgridLattSize()[i];
    
    return local_site(tmp_coord, Layout::logicalSize());
  }


  //! Returns the lattice site for some input node and linear index
  /*! This layout is a simple lexicographic lattice ordering */
  multi1d<int> siteCoords(int node, int linear)
  {
    multi1d<int> coord = crtesn(node, Layout::logicalSize());

    // Get the base (origins) of the absolute lattice coord
    coord *= Layout::subgridLattSize();
    
    // Find the coordinate within a node and accumulate
    // This is a lexicographic ordering
    coord += crtesn(linear, Layout::subgridLattSize());

    return coord;
  }



  //! Initializer for layout
  /*! This layout is a simple lexicographic lattice ordering */
  void create(const multi1d<int>& nrows)
  {
    if ( ! QDP_isInitialized() )
      QDP_error_exit("QDP is not initialized");

    if (nrows.size() != Nd)
      QDP_error_exit("dimension of lattice size not the same as the default");

    _layout.vol=1;
    _layout.nrow = nrows;
    _layout.cb_nrow = nrows;
    for(int i=0; i < Nd; ++i) 
      _layout.vol *= nrows[i];
  
#if defined(NO_MEM)
    if (_layout.vol > VOLUME)
      QDP_error_exit("Allocating a lattice size greater than compile time size: vol=%d",
		     _layout.vol);
#endif

    /* volume of checkerboard. Make sure global variable is set */
    _layout.nsubl = 1;
    _layout.vol_cb = _layout.vol / _layout.nsubl;

#if defined(DEBUG)
    fprintf(stderr,"vol=%d, nsubl=%d\n",_layout.vol,_layout.nsubl);
#endif

#if defined(DEBUG)
    cerr << "Initialize layout\n";
#endif
    // Crap to make the compiler happy with the C prototype
    unsigned int unsigned_nrow[Nd];
    for(int i=0; i < Nd; ++i)
      unsigned_nrow[i] = nrows[i];

    QMP_layout_grid(unsigned_nrow, Nd);


    // Pull out useful stuff
    const unsigned int* phys_size = QMP_get_logical_dimensions();
    const unsigned int* phys_coord = QMP_get_logical_coordinates();
    const unsigned int* subgrid_size = QMP_get_subgrid_dimensions();

    _layout.subgrid_vol = QMP_get_number_of_subgrid_sites();
    _layout.num_nodes = QMP_get_number_of_nodes();
    _layout.node_rank = QMP_get_node_number();

    _layout.subgrid_nrow.resize(Nd);
    _layout.logical_coord.resize(Nd);
    _layout.logical_size.resize(Nd);

    for(int i=0; i < Nd; ++i)
    {
      _layout.subgrid_nrow[i] = subgrid_size[i];
      _layout.logical_coord[i] = phys_coord[i];
      _layout.logical_size[i] = phys_size[i];
    }

    // Diagnostics
//  if (layout.primaryNode())
    {
      cerr << "Lattice initialized:\n";
      cerr << "  problem size =";
      for(int i=0; i < Nd; ++i)
	cerr << " " << _layout.nrow[i];
      cerr << endl;

      cerr << "  logical machine size =";
      for(int i=0; i < Nd; ++i)
	cerr << " " << _layout.logical_size[i];
      cerr << endl;

      cerr << "  logical node coord =";
      for(int i=0; i < Nd; ++i)
	cerr << " " << _layout.logical_coord[i];
      cerr << endl;

      cerr << "  subgrid size =";
      for(int i=0; i < Nd; ++i)
	cerr << " " << _layout.subgrid_nrow[i];
      cerr << endl;

      cerr << "  total volume = " << _layout.vol << endl;
      cerr << "  subgrid volume = " << _layout.subgrid_vol << endl;
    }


#if defined(DEBUG)
    cerr << "Create default subsets\n";
#endif
    InitDefaultSets();

    // Initialize RNG
    RNG::InitDefaultRNG();
  }
};

#elif defined(USE_CB2_LAYOUT)

#error "Using a 2 checkerboard (red/black) layout"

#elif defined(USE_CB32_LAYOUT)

#error "Using a 32 checkerboard layout"


#else

#error "no appropriate layout defined"

#endif


template<class T>
ostream& operator<<(ostream& s, const multi1d<T>& s1)
{
  for(int i=0; i < s1.size(); ++i)
    s << " " << s1[i];

  return s;
}


//-----------------------------------------------------------------------------
// Auxilliary operations
//! coord[mu]  <- mu  : fill with lattice coord in mu direction
LatticeInteger latticeCoordinate(int mu)
{
  LatticeInteger d;

  if (mu < 0 || mu >= Nd)
    QDP_error_exit("dimension out of bounds");

  const multi1d<int>& subgrid = Layout::subgridLattSize();
  const multi1d<int>& node_coord = Layout::nodeCoord();

  for(int i=0; i < Layout::subgridVol(); ++i) 
  {
    int site = Layout::lexicoSiteIndex(i);
    for(int k=0; k <= mu; ++k)
    {
      d.elem(i) = Integer(subgrid[k]*node_coord[k] + site % subgrid[k]).elem();
      site /= subgrid[k];
    } 
  }

  return d;
}


//-----------------------------------------------------------------------------
//! Constructor from an int function
void Set::make(const SetFunc& func)
{
  int nsubset_indices = func.numSubsets();

#if 1
  fprintf(stderr,"Set a subset: nsubset = %d\n",nsubset_indices);
#endif

  // This actually allocates the subsets
  sub.resize(nsubset_indices);

  // Create the space of the colorings of the lattice
  lat_color.resize(Layout::subgridVol());

  // Create the array holding the array of sitetable info
  sitetables.resize(nsubset_indices);

  // For a sanity check, set to some invalid value
  for(int site=0; site < Layout::subgridVol(); ++site)
    lat_color[site] = -1;

  // Loop over all sites determining their color
  for(int site=0; site < Layout::subgridVol(); ++site)
  {
    multi1d<int> coord = crtesn(site, Layout::subgridLattSize());

    for(int m=0; m<Nd; ++m)
      coord[m] += Layout::nodeCoord()[m]*Layout::subgridLattSize()[m];

    int node   = Layout::nodeNumber(coord);
    int linear = Layout::linearSiteIndex(coord);
    int icolor = func(coord);

    cerr << "site="<<site<<" coord="<<coord<<" node="<<node<<" linear="<<linear<<" col="<<icolor;
    cerr << endl;

    if (node != Layout::nodeNumber())
      QDP_error_exit("Set: found site with node outside current node!");

    lat_color[linear] = icolor;
  }

  // Loop over all sites and see if coloring properly set
  for(int site=0; site < Layout::subgridVol(); ++site)
  {
    if (lat_color[site] == -1)
      QDP_error_exit("Set: found site with coloring not set");
  }


  /*
   * Loop over the lexicographic sites.
   * Check if the linear sites are in a contiguous set.
   * This implementation only supports a single contiguous
   * block of sites.
   */
  for(int cb=0; cb < nsubset_indices; ++cb)
  {
    bool indexrep = true;
    int start = 0;
    int end = -1;

    // Always construct the sitetables. This could be moved into
    // the found_gap and only initialized if the interval method 
    // was not possible

    // First loop and see how many sites are needed
    int num_sitetable = 0;
    for(int linear=0; linear < Layout::subgridVol(); ++linear)
      if (lat_color[linear] == cb)
	++num_sitetable;

    // Now take the inverse of the lattice coloring to produce
    // the site list
    multi1d<int>& sitetable = sitetables[cb];
    sitetable.resize(num_sitetable);

    for(int linear=0, j=0; linear < Layout::subgridVol(); ++linear)
      if (lat_color[linear] == cb)
	sitetable[j++] = linear;


    sub[cb].make(start, end, indexrep, &(sitetables[cb]), cb);

#if 1
    fprintf(stderr,"Subset(%d): indexrep=%d start=%d end=%d\n",cb,indexrep,start,end);
#endif
  }
}
	  

//! Initializer for nearest neighbor shift
void NearestNeighborMap::make()
{
  //--------------------------------------
  // Setup the communication index arrays
  soffsets.resize(Nd, 2, Layout::subgridVol());

  /* Get the offsets needed for neighbour comm.
   * soffsets(direction,isign,position)
   *  where  isign    = +1 : plus direction
   *                  =  0 : negative direction
   *         cb       =  0 : even lattice (includes origin)
   *                  = +1 : odd lattice (does not include origin)
   * the offsets cotain the current site, i.e the neighbour for site i
   * is  soffsets(i,dir,mu) and NOT  i + soffset(..) 
   */
  const multi1d<int>& nrow = Layout::lattSize();
  const multi1d<int>& subgrid = Layout::subgridLattSize();
  const multi1d<int>& node_coord = Layout::nodeCoord();
  multi1d<int> node_offset(Nd);

  for(int m=0; m<Nd; ++m)
    node_offset[m] = node_coord[m]*subgrid[m];

  for(int site=0; site < Layout::vol(); ++site)
  {
    // Get the true grid of this site
    multi1d<int> coord = crtesn(site, nrow);

    // Site and node for this lattice site within the machine
    int ipos = Layout::linearSiteIndex(coord);
    int node = Layout::nodeNumber(coord);

    // If this is my node, then add it to my list
    if (Layout::nodeNumber() == node)
    {
//      <must get a new ipos within a node>

      for(int m=0; m<Nd; ++m)
      {
	multi1d<int> tmpcoord = coord;

	/* Neighbor in backward direction */
	tmpcoord[m] = (coord[m] - 1 + nrow[m]) % nrow[m];
	soffsets(m,0,ipos) = Layout::linearSiteIndex(tmpcoord);

	/* Neighbor in forward direction */
	tmpcoord[m] = (coord[m] + 1) % nrow[m];
	soffsets(m,1,ipos) = Layout::linearSiteIndex(tmpcoord);
      }
    }
  }

#if 0
  for(int m=0; m < Nd; ++m)
    for(int s=0; s < 2; ++s)
      for(int ipos=0; ipos < Layout::subgridVol(); ++ipos)
	fprintf(stderr,"soffsets(%d,%d,%d) = %d\n",ipos,s,m,soffsets(m,s,ipos));
#endif
}


//! Initializer for generic map constructor
void Map::make(const MapFunc& func)
{
  QMP_info("Map::make");

  //--------------------------------------
  // Setup the communication index arrays
  soffsets.resize(Layout::subgridVol());
  srcnode.resize(Layout::subgridVol());
  dstnode.resize(Layout::subgridVol());

  const int my_node = Layout::nodeNumber();

  // Loop over the sites on this node
  for(int linear=0; linear < Layout::subgridVol(); ++linear)
  {
    // Get the true lattice coord of this linear site index
    multi1d<int> coord = Layout::siteCoords(my_node, linear);

    // Source neighbor for this destination site
    multi1d<int> fcoord = func(coord,+1);

    // Destination neighbor receiving data from this site
    // This functions as the inverse map
    multi1d<int> bcoord = func(coord,-1);

    int fnode = Layout::nodeNumber(fcoord);
    int bnode = Layout::nodeNumber(bcoord);

    // Source linear site and node
    soffsets[linear] = Layout::linearSiteIndex(fcoord);
    srcnode[linear]  = fnode;

    // Destination node
    dstnode[linear]  = bnode;
  }

//  extern NmlWriter nml;

//  Write(nml,srcnode);
//  Write(nml,dstnode);

  // Return a list of the unique nodes in the list
  // NOTE: my_node is always included as a unique node, so one extra
  srcenodes = uniquify_list(srcnode);
  destnodes = uniquify_list(dstnode);

//  Write(nml,srcenodes);
//  Write(nml,destnodes);

  // Run through the lists and find the number for each unique node
  srcenodes_num.resize(srcenodes.size());
  destnodes_num.resize(destnodes.size());

  srcenodes_num = 0;
  destnodes_num = 0;

  for(int linear=0; linear < Layout::subgridVol(); ++linear)
  {
    int this_node = srcnode[linear];
    for(int i=0; i < srcenodes_num.size(); ++i)
    {
      if (srcenodes[i] == this_node)
      {
	srcenodes_num[i]++;
	break;
      }
    }

    this_node = dstnode[linear];
    for(int i=0; i < destnodes_num.size(); ++i)
    {
      if (destnodes[i] == this_node)
      {
	destnodes_num[i]++;
	break;
      }
    }
  }
  
//  Write(nml,srcenodes_num);
//  Write(nml,destnodes_num);


#if 1
  for(int linear=0; linear < Layout::subgridVol(); ++linear)
  {
    QMP_info("soffsets(%d) = %d",linear,soffsets(linear));
    QMP_info("srcnode(%d) = %d",linear,srcnode(linear));
    QMP_info("dstnode(%d) = %d",linear,dstnode(linear));
  }

  for(int i=0; i < destnodes.size(); ++i)
  {
    QMP_info("srcenodes(%d) = %d",i,srcenodes(i));
    QMP_info("destnodes(%d) = %d",i,destnodes(i));
    QMP_info("srcenodes_num(%d) = %d",i,srcenodes_num(i));
    QMP_info("destnodes_num(%d) = %d",i,destnodes_num(i));
  }
#endif

  QMP_info("exiting Map::make");
}


//------------------------------------------------------------------------
// Message passing convenience routines
//------------------------------------------------------------------------

namespace Internal
{
  // Nearest neighbor communication channels
  static QMP_msgmem_t request_msg[Nd][2];
  static QMP_msghandle_t request_mh[Nd][2];
  static QMP_msghandle_t mh_both[Nd];

  //! Slow send-receive (blocking)
  void
  sendRecvWait(void *send_buf, void *recv_buf, 
	       int count, int isign, int dir)
  {
#ifdef DEBUG
    QMP_info("starting a sendRecvWait, count=%d\n",count);
#endif

    QMP_msgmem_t msg[2] = {QMP_declare_msgmem(send_buf, count),
			   QMP_declare_msgmem(recv_buf, count)};
    QMP_msghandle_t mh_a[2] = {QMP_declare_send_relative(msg[0], dir, isign, 0),
			       QMP_declare_receive_relative(msg[1], dir, -isign, 0)};
    QMP_msghandle_t mh = QMP_declare_multiple(mh_a, 2);

    QMP_start(mh);
    QMP_wait(mh);

    QMP_free_msghandle(mh_a[1]);
    QMP_free_msghandle(mh_a[0]);
    QMP_free_msghandle(mh);
    QMP_free_msgmem(msg[1]);
    QMP_free_msgmem(msg[0]);

#ifdef DEBUG
    QMP_info("finished a sendRecvWait\n");
#endif
  }


  //! Fast send-receive (non-blocking)
  void
  sendRecv(void *send_buf, void *recv_buf, 
	   int count, int isign0, int dir)
  {
#ifdef DEBUG
    QMP_info("starting a sendRecv, count=%d, isign=%d dir=%d\n",
	     count,isign,dir);
#endif

    int isign = (isign0 > 0) ? 1 : -1;

    request_msg[dir][0] = QMP_declare_msgmem(send_buf, count);
    request_msg[dir][1] = QMP_declare_msgmem(recv_buf, count);
    request_mh[dir][1] = QMP_declare_send_relative(request_msg[dir][0], dir, isign, 0);
    request_mh[dir][0] = QMP_declare_receive_relative(request_msg[dir][1], dir, -isign, 0);
    mh_both[dir] = QMP_declare_multiple(request_mh[dir], 2);

    if (QMP_start(mh_both[dir]) != QMP_SUCCESS)
      QMP_error_exit("QMP_create_physical_topology failed\n");

#ifdef DEBUG
    QMP_info("finished a sendRecv\n");
#endif
  }

  //! Wait on send-receive (now blocks)
  void
  wait(int dir)
  {
#ifdef DEBUG
    QMP_info("starting a wait\n");
#endif
    
    QMP_wait(mh_both[dir]);

    QMP_free_msghandle(request_mh[dir][1]);
    QMP_free_msghandle(request_mh[dir][0]);
    QMP_free_msghandle(mh_both[dir]);
    QMP_free_msgmem(request_msg[dir][1]);
    QMP_free_msgmem(request_msg[dir][0]);

#ifdef DEBUG
    QMP_info("finished a wait\n");
#endif
  }


  //! Send to another node (wait)
  void 
  sendToWait(void *send_buf, int dest_node, int count)
  {
#ifdef DEBUG
    QMP_info("starting a sendToWait, count=%d, destnode=%d\n", count,dest_node);
#endif

    QMP_msgmem_t request_msg = QMP_declare_msgmem(send_buf, count);
    QMP_msghandle_t request_mh = QMP_declare_send_to(request_msg, dest_node, 0);

    if (QMP_start(request_mh) != QMP_SUCCESS)
      QMP_error_exit("sendToWait failed\n");

    QMP_wait(request_mh);

    QMP_free_msghandle(request_mh);
    QMP_free_msgmem(request_msg);

#ifdef DEBUG
    QMP_info("finished a sendToWait\n");
#endif
  }

  //! Receive from another node (wait)
  void 
  recvFromWait(void *recv_buf, int srce_node, int count)
  {
#ifdef DEBUG
    QMP_info("starting a recvFromWait, count=%d, srcenode=%d\n", count, srce_node);
#endif

    QMP_msgmem_t request_msg = QMP_declare_msgmem(recv_buf, count);
    QMP_msghandle_t request_mh = QMP_declare_receive_from(request_msg, srce_node, 0);

    if (QMP_start(request_mh) != QMP_SUCCESS)
      QMP_error_exit("recvFromWait failed\n");

    QMP_wait(request_mh);

    QMP_free_msghandle(request_mh);
    QMP_free_msgmem(request_msg);

#ifdef DEBUG
    QMP_info("finished a recvFromWait\n");
#endif
  }

};

QDP_END_NAMESPACE();
