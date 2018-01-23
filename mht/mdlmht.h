
/*********************************************************************
 * FILE: mdlmht.H                                                    *
 *                                                                   *
 * AUTHOR: Matthew Miller (mlm)                                      *
 *                                                                   *
 * HISTORY:                                                          *
 *    6 JUN 93 -- (mlm) created                                      *
 *                                                                   *
 * CONTENTS:                                                         *
 *                                                                   *
 *   Base classes for multiple hypothesis testing with multiple      *
 *   target models.                                                  *
 *                                                                   *
 *   To make an application that employs the mht algorithm with one  *
 *   or more target models, you define one or more object classes    *
 *   that inherit from each of the classes MODEL, MDL_STATE,         *
 *   MDL_REPORT, and MDL_MHT.  Each of these classes has a set of    *
 *   virtual functions that should be defined by the application     *
 *   programmer for his or her specific application.                 *
 *                                                                   *
 *   The basic meanings of these four classes are as follows:        *
 *                                                                   *
 *     MODEL subclasses define possible behaviors of a target.       *
 *       These are the target models in the application.  Target     *
 *       behaviors are specified by code that estimates a target's   *
 *       current state, given an estimate of it's previous state     *
 *       and (possibly) a noisy reported measurement.                *
 *                                                                   *
 *     MDL_STATE subclasses contain descriptions of an estimated     *
 *       state of a target (e.g. where it is).  Each MDL_STATE       *
 *       object is tied to one, specific MODEL object.  The MODEL    *
 *       object will be used to estimate one or more possible        *
 *       future states of the target.                                *
 *                                                                   *
 *     MDL_REPORT subclasses contain descriptions of reported        *
 *       measurements (e.g. blips from radar, or edgels in an        *
 *       image).                                                     *
 *                                                                   *
 *     The MDL_MHT subclass (there's usually only one per program)   *
 *       contains a list of models that might describe the behavior  *
 *       of targets when they're first detected, and some variables  *
 *       used to control pruning in the mht algorithm.  It then ties *
 *       all these things together and uses them in the mht          *
 *       algorithm to track targets in noisy data.                   *
 *                                                                   *
 *   Each of these base classes is described in detail below.        *
 *                                                                   *
 *                           MODEL                                   *
 *                                                                   *
 *   The basic function of a MODEL subclass is the generation of     *
 *   new state estimates from old estimates and reported             *
 *   measurements.  In each iteration of the mht algorithm a MODEL   *
 *   will be called to generate 0 or more new estimates resulting    *
 *   from one estimate.                                              *
 *                                                                   *
 *   The state estimates that a MODEL generates may be assigned to   *
 *   other MODEL's.  This indicates that the proper model for the    *
 *   target's behavior might have changed.  For example, a system    *
 *   for tracking planes might have two target MODEL's: one that     *
 *   assumes the plane is flying straight, and one that assumes it's *
 *   turning.  The MODEL for straight flight should generate two     *
 *   state estimates for each target in each iteration: one that     *
 *   assumes the plane continued flying straight (and is assigned    *
 *   to the straight flight MODEL), and one that assumes the plane   *
 *   began turning (and is assigned to the turning flight MODEL).    *
 *                                                                   *
 *   The MODEL constructor takes no arguments.                       *
 *                                                                   *
 *   The virtual functions to be redefined in a MODEL subclass are:  *
 *                                                                   *
 *     int beginNewStates( MDL_STATE *s, MDL_REPORT *r )             *
 *                                                                   *
 *       This is called when the new state estimates will be         *
 *       required.  It is given a pointer to the old state, and a    *
 *       pointer to a reported measurement.  It should return the    *
 *       number of different state estimates that will be generated  *
 *       by the model from the given state and report.               *
 *                                                                   *
 *       If the MDL_STATE pointer is null, then state estimates      *
 *       should be given for the beginning of a new target track,    *
 *       assuming that the report is the first measurement of that   *
 *       target.                                                     *
 *                                                                   *
 *       If the MDL_REPORT pointer is null, then state estimates     *
 *       should be based purely on the possible behaviors of the     *
 *       target after the given state, assuming that the detector    *
 *       failed to measure the target.                               *
 *                                                                   *
 *       When this routine is called, the MODEL object can set up    *
 *       any internal variables it will be using for all the new     *
 *       state estimates.                                            *
 *                                                                   *
 *     MDL_STATE *getNewState( int i, MDL_STATE *s, MDL_REPORT *r )  *
 *                                                                   *
 *       This is called to get the i'th state estimate resulting     *
 *       from the given state and report.  It should return a        *
 *       pointer to a MDL_STATE object that describes the estimate,  *
 *       or it should return null.                                   *
 *                                                                   *
 *       The number of times that this routine is called is exactly  *
 *       the number that was returned by the last call to            *
 *       beginNewStates().  In other words, this is used in a loop   *
 *       in the following way:                                       *
 *                                                                   *
 *          numNewStates = beginNewStates( s, r );                   *
 *          for( i = 0; i < numNewStates; i++ )                      *
 *          {                                                        *
 *            newState = getNewState( i, s, r );                     *
 *            if( newState != 0 )                                    *
 *            {                                                      *
 *              .                                                    *
 *              .                                                    *
 *              .                                                    *
 *            }                                                      *
 *          }                                                        *
 *                                                                   *
 *       It should return null if the i'th state shouldn't be        *
 *       considered for some reason.                                 *
 *                                                                   *
 *       For example, in the MODEL for straight flight used by the   *
 *       plane-tracking program described above, beginNewStates()    *
 *       might always return 2.  When getNewState() is called with   *
 *       i = 0, it returns a state estimate based on the assumption  *
 *       that the plane continued flying straight.  When it is       *
 *       called with i = 1, it returns and estimate based on the     *
 *       assumption that the plane began turning.  It might be that, *
 *       given the plane's state estimate, and the reported          *
 *       measurement, it's extremely unlikely that it began turning. *
 *       In this case, getNewState() might decide that the turning   *
 *       estimate is not worth considering, so it returns null       *
 *       instead.                                                    *
 *                                                                   *
 *     void endNewStates()                                           *
 *                                                                   *
 *       This is called after the above loop is finished.  It's      *
 *       there to give you a chance to clean up any stuff done       *
 *       during the calls to beginNewStates() and getNewState().     *
 *                                                                   *
 *     double getEndLogLikelihood( MDL_STATE *s )                    *
 *                                                                   *
 *       This should return the log of the likelihood that a target  *
 *       track ends (the target disappears) after it was in the      *
 *       given state.                                                *
 *                                                                   *
 *     double getContinueLogLikelihood( MDL_STATE *s )               *
 *                                                                   *
 *       This is the opposite of getEndLogLikelihood().  If          *
 *       getEndLogLikelihood() returns log( P ), then                *
 *       getContinueLogLikelihood() should return log( 1 - P ).      *
 *                                                                   *
 *     double getSkipLogLikelihood( MDL_STATE *s )                   *
 *                                                                   *
 *       This should return the log of the likelihood that a target  *
 *       was not detected, even though it didn't end, after being in *
 *       state s on the previous scan.                               *
 *                                                                   *
 *     double getDetectLogLikelihood( MDL_STATE *s )                 *
 *                                                                   *
 *       This is the opposite of getSkipLogLikelihood().             *
 *                                                                   *
 *                            MDL_STATE                              *
 *                                                                   *
 *   A MDL_STATE subclass contains a description of a state          *
 *   estimate.  The details of that description are dependent on the *
 *   application, and the model that the MDL_STATE is intended to    *
 *   be used with.  The code in mdlmht.H and mdlmht.C doesn't care   *
 *   about the state estimate itself -- only about the model that    *
 *   the state is used with and the likelihood that the state is     *
 *   the true successor to the previous state.                       *
 *                                                                   *
 *   The constructor for MDL_STATE's takes a pointer to a MODEL      *
 *   object as an argument.  The pointer specifies the model that    *
 *   should be used to make estimates of the states that come after  *
 *   this one.                                                       *
 *                                                                   *
 *   For example, in our plane-tracking program (see above), any     *
 *   state that was based on an assumption of straight flight should *
 *   contain a pointer to the straight-flight MODEL, and any state   *
 *   based on an assumption of turning flight should contain a       *
 *   pointer to the turning-flight MODEL.                            *
 *                                                                   *
 *   MDL_STATE's have the following public member functions:         *
 *                                                                   *
 *     MODEL *getMdl() const                                         *
 *                                                                   *
 *       Returns the MODEL pointer that the MDL_STATE was            *
 *       constructed with.                                           *
 *                                                                   *
 *   The virtual functions to be redefined in a MDL_STATE subclass   *
 *   are:                                                            *
 *                                                                   *
 *     double getLogLikelihood()                                     *
 *                                                                   *
 *       Return the log of the likelihood that this state truly      *
 *       follows the one that it was generated from.                 *
 *                                                                   *
 *       That is, suppose MDL_STATE s1 was generated by              *
 *       MODEL::getNewState() as follows:                            *
 *                                                                   *
 *         s1 = s0->getMdl()->getNewState( s0, r )                   *
 *                                                                   *
 *       Then s1->getLogLikelihood() should return the log of the    *
 *       likelihood that the estimated state described in s1 is      *
 *       true, given that s0 is true and r is a noisy measurement    *
 *       of the target in question.                                  *
 *                                                                   *
 *                           MDL_REPORT                              *
 *                                                                   *
 *   A MDL_REPORT contains a description of a reported measurement.  *
 *                                                                   *
 *   The following virtual function should be redefined:             *
 *                                                                   *
 *     virtual double getFAlarmLogLikelihood()                       *
 *                                                                   *
 *       This should return the log of the likelihood that the       *
 *       MDL_REPORT was a false alarm.                               *
 *                                                                   *
 *                                                                   *
 *                            MDL_MHT                                *
 *                                                                   *
 *   Generally, a given application has only one MDL_MHT object.     *
 *   It ties together all the information about the target tracking  *
 *   problem, and implements the MHT algorithm to solve it.          *
 *                                                                   *
 *   The constructor for MDL_MHT takes three arguments.  They are:   *
 *                                                                   *
 *     int maxDepth -- maximum depth that any given track tree will  *
 *       be allowed to reach (n-scanback pruning).                   *
 *                                                                   *
 *     double minGHypoRatio -- minimum ratio of the likelihoods of   *
 *       the least likely and the most likely group hypotheses kept  *
 *       (ratio pruning).                                            *
 *                                                                   *
 *     int maxGHypos -- maximum number of group hypotheses kept      *
 *       (k-best pruning).                                           *
 *                                                                   *
 *   The constructors for subclasses of MDL_MHT should also set the  *
 *   following protected member variables:                           *
 *                                                                   *
 *     ptrDLIST_OF< MODEL > m_modelList -- linked list of pointers   *
 *       to MODEL objects.  Whenever a measurement is reported to    *
 *       the MDL_MHT object, each MODEL object on this list is given *
 *       a chance to initiate tracks with it.  The MODEL object's    *
 *       beginNewState(), getNewState(), and endNewState() member    *
 *       functions are called as described above, using a MDL_STATE  *
 *       pointer of null.                                            *
 *                                                                   *
 *   The key member function of a MDL_MHT object is scan(), which    *
 *   it inherits from the MHT class.  See "mht.H" for a discussion   *
 *   of this function.                                               *
 *                                                                   *
 *   The following virtual member functions can be redefined for the *
 *   specific application:                                           *
 *                                                                   *
 *     void measure()                                                *
 *       This should read one iteration of data, using the           *
 *       protected function installReport() (inherited from MHT) to  *
 *       make a list of measurement reports.                         *
 *                                                                   *
 *     void measureAndValidate()                                     *
 *       This shouldn't be redefined except under unusual            *
 *       circumstances.  It is first defined in MHT.  See "mht.H"    *
 *       for a discussion of this function.                          *
 *                                                                   *
 *     void startTrack( int id, int t, MDL_STATE *s, MDL_REPORT *r ) *
 *       This is called during scan(), whenever the MDL_MHT object   *
 *       decides that a track has started.  It is given a unique     *
 *       track id, which will be used to identify the track from     *
 *       now on; a timestamp, which is the scan number of the scan   *
 *       that read in the measurement that started this track; and   *
 *       the state estimate and measurement report for the beginning *
 *       of the track.                                               *
 *                                                                   *
 *     void continueTrack( ... )                                     *
 *       This is called during scan(), whenever the MDL_MHT object   *
 *       decides that a track continued and a measurement of it was  *
 *       reported.  The arguments are the same as for startTrack().  *
 *                                                                   *
 *     void skipTrack( ... )                                         *
 *       This is called during scan(), whenever the MDL_MHT object   *
 *       decides that a track continued but no measurement of it     *
 *       was reported.  The arguments are the same as for            *
 *       startTrack(), except that no MDL_REPORT is given.           *
 *                                                                   *
 *     void endTrack( int id, int t )                                *
 *       This is called during scan(), when the MDL_MHT object       *
 *       decides that track number 'id' ended during scan number     *
 *       't'.                                                        *
 *                                                                   *
 *     void falseAlarm( int t, MDL_REPORT *r )                       *
 *       This is called during scan(), when the MDL_MHT object       *
 *       decides that report '*r' was a false alarm received during  *
 *       scan number 't'.                                            *
 *                                                                   *
 *   Note that the time stamp, t, in the above routines, will        *
 *   generally be earlier than the time at which the routine is      *
 *   called.  At most, the difference between t and the current      *
 *   time will be maxDepth.                                          *
 *                                                                   *
 * IMPLEMENTATION NOTES:                                             *
 *                                                                   *
 *   This stuff inherits from the classes in "mht.H", which          *
 *   implements all the "multiple" part of the multiple hypothesis   *
 *   testing algorithm.  That is, it handles all the interaction     *
 *   of multiple track trees with each other, but it doesn't know    *
 *   how to actually grow them.  This means that the mdlmht code     *
 *   need implement only track tree growth -- it doesn't have to     *
 *   worry about global hypotheses or pruning or any of that stuff.  *
 *                                                                   *
 *   In the track trees grown in a MDL_MHT object, there are seven   *
 *   basic types of track hypotheses (track tree nodes).  They are   *
 *                                                                   *
 *     ROOT -- roots of track trees                                  *
 *     DUMMY -- used for various reasons                             *
 *     FALARM -- false alarms                                        *
 *     START -- start of a track                                     *
 *     CONTINUE -- continuation of a track, with measurement report  *
 *     SKIP -- continuation of a track, without measurement report   *
 *     END -- end of a track                                         *
 *                                                                   *
 *   The constructor for each of these types of nodes takes all the  *
 *   arguments required to compute the likelihood of that node.  The *
 *   likelihood is computed at the time of construction.             *
 *                                                                   *
 *   Each of these types of nodes has two member functions for       *
 *   growing their children:                                         *
 *                                                                   *
 *     makeDefaultChildren()                                         *
 *       This makes the children that don't depend on any reported   *
 *       measurement.  SKIP nodes and END nodes are examples of      *
 *       such children.                                              *
 *                                                                   *
 *     makeChildrenFor( MDL_REPORT *r )                              *
 *       This makes the children that DO depend on a reported        *
 *       measurement.  Generally, they plug the measurement into a   *
 *       MODEL and make a child for each state estimate that comes   *
 *       out.                                                        *
 *                                                                   *
 *   Here's how track trees grow:                                    *
 *                                                                   *
 *   For each report in a scan, a new track tree is created, which   *
 *   indicates the various possible interpretations of the           *
 *   measurement.  The tree consists of a ROOT with two or more      *
 *   children, as follows:                                           *
 *                                                                   *
 *     A DUMMY node, to be used if the report is assigned to an      *
 *       existing track tree.  This node is not linked to the        *
 *       report.                                                     *
 *                                                                   *
 *     An FALARM node, indicating the hypothesis that the report is  *
 *       a false alarm.                                              *
 *                                                                   *
 *     Zero or more START nodes, indicating various ways that the    *
 *       report might start a new track.                             *
 *                                                                   *
 *   In subsequent scans, nodes grow children as follows:            *
 *                                                                   *
 *     Each DUMMY node, FALARM node, and END node grows only one     *
 *       child: a DUMMY node.  The child has the same likelihood as  *
 *       its parent.                                                 *
 *                                                                   *
 *     Each START node, CONTINUE node, and SKIP node grows one END   *
 *       child, zero or more SKIP children, and zero or more         *
 *       CONTINUE children.  The SKIP children are made by plugging  *
 *       the current state estimate into its MODEL with no report.   *
 *       The CONTINUE children are made by plugging the current      *
 *       state estimate into its MODEL with each report found by     *
 *       measure().                                                  *
 *                                                                   *
 * ----------------------------------------------------------------- *
 *                                                                   *
 *             Copyright (c) 1993, NEC Research Institute            *
 *                       All Rights Reserved.                        *
 *                                                                   *
 *   Permission to use, copy, and modify this software and its       *
 *   documentation is hereby granted only under the following terms  *
 *   and conditions.  Both the above copyright notice and this       *
 *   permission notice must appear in all copies of the software,    *
 *   derivative works or modified versions, and any portions         *
 *   thereof, and both notices must appear in supporting             *
 *   documentation.                                                  *
 *                                                                   *
 *   Correspondence should be directed to NEC at:                    *
 *                                                                   *
 *                     Ingemar J. Cox                                *
 *                                                                   *
 *                     NEC Research Institute                        *
 *                     4 Independence Way                            *
 *                     Princeton                                     *
 *                     NJ 08540                                      *
 *                                                                   *
 *                     phone:  609 951 2722                          *
 *                     fax:  609 951 2482                            *
 *                     email:  ingemar@research.nj.nec.com (Inet)    *
 *                                                                   *
 *********************************************************************/

#ifndef MDLMHT_H
#define MDLMHT_H

#include "mht.h"
#include "corner.h"		// for CORNER class
#include <list>			// for std::list<>

/*-------------------------------------------------------------------*
 | Stuff defined in this file
 *-------------------------------------------------------------------*/

class MODEL;
template<class> class MDL_MHT;
class MDL_STATE;
class MDL_REPORT;
template<class> class MDL_T_HYPO;
template<class> class MDL_ROOT_T_HYPO;
template<class> class MDL_FALARM_T_HYPO;
template<class> class MDL_DUMMY_T_HYPO;
template<class> class MDL_START_T_HYPO;
template<class> class MDL_CONTINUE_T_HYPO;
template<class> class MDL_SKIP_T_HYPO;
template<class> class MDL_END_T_HYPO;

/*-------------------------------------------------------------------*
 | MODEL -- base class for target behavior models
 *-------------------------------------------------------------------*/

class MODEL
{
public:

    MODEL() {}
    virtual ~MODEL() {}

    virtual int beginNewStates( MDL_STATE *, MDL_REPORT * )
    {
        return 0;
    }
    virtual MDL_STATE *getNewState( int, MDL_STATE *, MDL_REPORT * )
    {
        return 0;
    }
    virtual void endNewStates() {}

    virtual double getEndLogLikelihood( MDL_STATE * )
    {
        return 0;
    }
    virtual double getContinueLogLikelihood( MDL_STATE * )
    {
        return -INFINITY;
    }
    virtual double getSkipLogLikelihood( MDL_STATE * )
    {
        return -INFINITY;
    }
    virtual double getDetectLogLikelihood( MDL_STATE * )
    {
        return 0;
    }
};

/*-------------------------------------------------------------------*
 | MDL_STATE -- describes a state estimate
 *-------------------------------------------------------------------*/

class MDL_STATE
{
    //friend class MDL_MHT;
    //friend class MDL_ROOT_T_HYPO;
    //friend class MDL_FALARM_T_HYPO;
    //friend class MDL_DUMMY_T_HYPO<CORNER>;
    template<class> friend class MDL_CONTINUE_T_HYPO;
    //friend class MDL_START_T_HYPO;
    //friend class MDL_SKIP_T_HYPO;
    //friend class MDL_END_T_HYPO;

private:

    MODEL *m_mdl;                        // model to use to find
    //   states that come after
    //   this one

protected:

    MDL_STATE( MODEL *mdl ): m_mdl( mdl ) {}
    virtual ~MDL_STATE() {}

public:

    MODEL *getMdl() const
    {
        return m_mdl;
    }

    virtual double getLogLikelihood()
    {
        return -INFINITY;
    }

    virtual void print()
    {
        std::cout << "S:" << (void *)this;
    }
};

/*-------------------------------------------------------------------*
 | MDL_REPORT -- measurement report
 *-------------------------------------------------------------------*/

class MDL_REPORT: public REPORT
{
public:

    MDL_REPORT() {}
    virtual ~MDL_REPORT() {}

    virtual double getFalarmLogLikelihood()
    {
        return -INFINITY;
    }
};

/*-------------------------------------------------------------------*
 | MDL_MHT -- model-based MHT class
 *-------------------------------------------------------------------*/

template<class CORNER>
class MDL_MHT: public MHT<CORNER>
{
    friend class MDL_ROOT_T_HYPO<CORNER>;
    friend class MDL_FALARM_T_HYPO<CORNER>;
    friend class MDL_DUMMY_T_HYPO<CORNER>;
    friend class MDL_START_T_HYPO<CORNER>;
    friend class MDL_CONTINUE_T_HYPO<CORNER>;
    friend class MDL_SKIP_T_HYPO<CORNER>;
    friend class MDL_END_T_HYPO<CORNER>;

protected:

    ptrDLIST_OF< MODEL > m_modelList;

public:

    MDL_MHT( int maxDepth, double minGHypoRatio, int maxGHypos ):
        MHT<CORNER>( maxDepth, minGHypoRatio, maxGHypos ),
        m_modelList()
    {
    }

    virtual ~MDL_MHT() {}

protected:

    virtual void measure(const std::list<CORNER> &/*newReports*/,
                         const double &/*deltaT*/) {}

    /*-------------------------------------------------------------------*
     | MDL_MHT::measureAndValidate() -- collect reports of measurements
     |                                  and grow track trees
     *-------------------------------------------------------------------*/
    virtual void measureAndValidate(const std::list<CORNER> &newReports, const double &deltaT)
    {


        PTR_INTO_ptrDLIST_OF< T_HYPO > tHypoPtr;
        MDL_T_HYPO<CORNER> *tHypo;
        PTR_INTO_iDLIST_OF< REPORT > reportPtr;
        MDL_REPORT *report;
        MDL_ROOT_T_HYPO<CORNER> *root;

        /* get reports of measurements */
        measure(newReports, deltaT);

        /* loop through all the active track hypotheses (leaves of the track
       trees), making children for each one */
        LOOP_DLIST( tHypoPtr, this->m_activeTHypoList )
        {
            tHypo = (MDL_T_HYPO<CORNER> *)tHypoPtr.get();

            tHypo->makeDefaultChildren();

            LOOP_DLIST( reportPtr, this->m_newReportList )
            {
                report = (MDL_REPORT *)reportPtr.get();
                tHypo->makeChildrenFor( report );
            }
        }

        /* make a new track tree for each reported measurement */
        LOOP_DLIST( reportPtr, this->m_newReportList )
        {
            report = (MDL_REPORT *)reportPtr.get();

            root = new MDL_ROOT_T_HYPO<CORNER>( this );
            this->installTree( root, -1 );
            root->makeDefaultChildren();
            root->makeChildrenFor( report );
        }
    }

    virtual void startTrack( int, int,
                             MDL_STATE *, MDL_REPORT * )
    {
    }

    virtual void continueTrack( int, int,
                                MDL_STATE *, MDL_REPORT * )
    {
    }

    virtual void skipTrack( int, int, MDL_STATE * )
    {
    }

    virtual void endTrack( int, int )
    {
    }

    virtual void falseAlarm( int, MDL_REPORT * )
    {
    }
};

/*-------------------------------------------------------------------*
 | Classes for track hypotheses (nodes on track tree)
 |
 | Because the behavior of these nodes is pretty similar to each
 | other, I put them into a class hierarchy and reduced the amount
 | of code.  This hierarchy doesn't really follow the rule that a
 | class which inherits from a base class should be considered as a
 | more specific example of that class (the so-called "isa" rule).
 |
 | The hierarchy:
 |
 |  MDL_T_HYPO (contains virtual definitions of routines shared by all)
 |    MDL_ROOT_T_HYPO
 |    MDL_DUMMY_T_HYPO
 |      MDL_FALARM_T_HYPO
 |      MDL_END_T_HYPO
 |    MDL_CONTINUE_T_HYPO
 |      MDL_START_T_HYPO
 |      MDL_SKIP_T_HYPO
 *-------------------------------------------------------------------*/

template<class CORNER>
class MDL_T_HYPO: public T_HYPO
{
    friend class MDL_MHT<CORNER>;

protected:

    MDL_MHT<CORNER> *m_mdlMht;           // MDL_MHT object that this
                                         //   tree is part of

protected:

    MDL_T_HYPO( MDL_MHT<CORNER> *mdlMht ): T_HYPO(), m_mdlMht( mdlMht ) {}
    MDL_T_HYPO( MDL_MHT<CORNER> *mdlMht, MDL_REPORT *report ):
        T_HYPO( report ),
        m_mdlMht( mdlMht )
    {
    }

    virtual ~MDL_T_HYPO() {}

    virtual void makeDefaultChildren() {}
    virtual void makeChildrenFor( MDL_REPORT * ) {}

public:

    virtual MDL_STATE *getState()
    {
        return 0;
    }
};

template<class CORNER>
class MDL_ROOT_T_HYPO: public MDL_T_HYPO<CORNER>
{
    friend class MDL_MHT<CORNER>;
    friend class MDL_FALARM_T_HYPO<CORNER>;
    friend class MDL_DUMMY_T_HYPO<CORNER>;
    friend class MDL_START_T_HYPO<CORNER>;
    friend class MDL_CONTINUE_T_HYPO<CORNER>;
    friend class MDL_SKIP_T_HYPO<CORNER>;
    friend class MDL_END_T_HYPO<CORNER>;

protected:

    MDL_ROOT_T_HYPO( MDL_MHT<CORNER> *mdlMht ):
        MDL_T_HYPO<CORNER>( mdlMht )
    {
        this->m_endsTrack = 0;
        this->m_mustVerify = 0;
        this->m_logLikelihood = 0;
    }

    /*-------------------------------------------------------------------*
     | MDL_ROOT_T_HYPO::makeDefaultChildren() -- make the default
     |                                           children of a ROOT node
     |
     | This makes the children that are not linked to a report.
     *-------------------------------------------------------------------*/
    virtual void makeDefaultChildren()
    {
        this->installChild( new MDL_DUMMY_T_HYPO<CORNER>( this->m_mdlMht ) );
    }

    /*-------------------------------------------------------------------*
     | MDL_ROOT_T_HYPO::makeChildrenFor() -- make children of a ROOT node
     |
     | This makes the children that ARE linked to a report.
     *-------------------------------------------------------------------*/
    virtual void makeChildrenFor( MDL_REPORT *report )
    {
        PTR_INTO_ptrDLIST_OF< MODEL > modelPtr;
        MODEL *mdl;
        MDL_STATE *state;
        int numStartStates;
        int i;

        this->installChild(
                    new MDL_FALARM_T_HYPO<CORNER>( this->m_mdlMht, report ) );

        LOOP_DLIST( modelPtr, this->m_mdlMht->m_modelList )
        {
            mdl = modelPtr.get();

            numStartStates = mdl->beginNewStates( 0, report );

            for( i = 0; i < numStartStates; i++ )
            {
                state = mdl->getNewState( i, 0, report );
                if( state != 0 )
                    this->installChild( new MDL_START_T_HYPO<CORNER>( this->m_mdlMht,
                                                                state,
                                                                report ) );
            }

            mdl->endNewStates();
        }
    }

public:

    virtual void print()
    {
        std::cout<<"T:"
                << this->getTrackStamp()
                << "("<<(void *)this<<")"
                << "(root:"
                << this->m_logLikelihood<<")";
    }
};

template<class CORNER>
class MDL_DUMMY_T_HYPO: public MDL_T_HYPO<CORNER>
{
    friend class MDL_MHT<CORNER>;
    friend class MDL_ROOT_T_HYPO<CORNER>;
    friend class MDL_FALARM_T_HYPO<CORNER>;
    friend class MDL_START_T_HYPO<CORNER>;
    friend class MDL_CONTINUE_T_HYPO<CORNER>;
    friend class MDL_SKIP_T_HYPO<CORNER>;
    friend class MDL_END_T_HYPO<CORNER>;

protected:

    MDL_DUMMY_T_HYPO( MDL_MHT<CORNER> *mdlMht, MDL_REPORT *report ):
        MDL_T_HYPO<CORNER>( mdlMht, report )
    {
    }

    MDL_DUMMY_T_HYPO( MDL_MHT<CORNER> *mdlMht, double logLikelihood = 0 ):
        MDL_T_HYPO<CORNER>( mdlMht )
    {
        this->m_endsTrack = 1;
        this->m_mustVerify = 0;
        this->m_logLikelihood = logLikelihood;
    }

    virtual ~MDL_DUMMY_T_HYPO() {}

    /*-------------------------------------------------------------------*
     | MDL_DUMMY_T_HYPO::makeDefaultChildren() -- make default children
     |                                            for a DUMMY node
     |
     | This makes the children that are not linked to a report.
     *-------------------------------------------------------------------*/
    virtual void makeDefaultChildren()
    {
        this->installChild( new MDL_DUMMY_T_HYPO<CORNER>(
                          this->m_mdlMht,
                          this->m_logLikelihood ) );
    }

public:

    virtual void print()
    {
        std::cout

                << "T:" << this->getTrackStamp()
                << "(" << (void *)this << ")"
                << "(dummy:" << this->m_logLikelihood << ")";
    }
};

template<class CORNER>
class MDL_FALARM_T_HYPO: public MDL_DUMMY_T_HYPO<CORNER>
{
    friend class MDL_MHT<CORNER>;
    friend class MDL_ROOT_T_HYPO<CORNER>;
    friend class MDL_DUMMY_T_HYPO<CORNER>;
    friend class MDL_START_T_HYPO<CORNER>;
    friend class MDL_CONTINUE_T_HYPO<CORNER>;
    friend class MDL_SKIP_T_HYPO<CORNER>;
    friend class MDL_END_T_HYPO<CORNER>;

protected:

    MDL_FALARM_T_HYPO( MDL_MHT<CORNER> *mdlMht, MDL_REPORT *report ):
        MDL_DUMMY_T_HYPO<CORNER>( mdlMht, report )
    {
        this->m_endsTrack = 1;
        this->m_mustVerify = 1;
        this->m_logLikelihood = report->getFalarmLogLikelihood();
//      cout << "FalseAlarm of likelihood = " << m_logLikelihood<<endl;
    }

    virtual void verify()
    {
        this->m_mdlMht->falseAlarm(
                    this->getTimeStamp(),
                    (MDL_REPORT *)this->getReport() );
    }

public:

    virtual void print()
    {
        std::cout << "T:[";
        this->getReport()->print();
        std::cout << "](falarm:"
                  <<this->m_logLikelihood<<")";
    }
};

template<class CORNER>
class MDL_CONTINUE_T_HYPO: public MDL_T_HYPO<CORNER>
{
    friend class MDL_MHT<CORNER>;
    friend class MDL_ROOT_T_HYPO<CORNER>;
    friend class MDL_FALARM_T_HYPO<CORNER>;
    friend class MDL_DUMMY_T_HYPO<CORNER>;
    friend class MDL_START_T_HYPO<CORNER>;
    friend class MDL_SKIP_T_HYPO<CORNER>;
    friend class MDL_END_T_HYPO<CORNER>;

protected:

    MDL_STATE *m_state;                  // state estimate

protected:

    MDL_CONTINUE_T_HYPO( MDL_MHT<CORNER> *mdlMht ):
        MDL_T_HYPO<CORNER>( mdlMht ),
        m_state( 0 )
    {
    }

    MDL_CONTINUE_T_HYPO( MDL_MHT<CORNER> *mdlMht, MDL_REPORT *report ):
        MDL_T_HYPO<CORNER>( mdlMht, report ),
        m_state( 0 )
    {
    }

    MDL_CONTINUE_T_HYPO( MDL_MHT<CORNER> *mdlMht,
                         double trackLogLikelihood,
                         double continueLogLikelihood,
                         double detectLogLikelihood,
                         MDL_STATE *state, MDL_REPORT *report ):
        MDL_T_HYPO<CORNER>( mdlMht, report ),
        m_state( state )
    {
        this->m_endsTrack = 0;
        this->m_mustVerify = 1;
        this->m_logLikelihood = trackLogLikelihood +
                          continueLogLikelihood +
                          detectLogLikelihood +
                          m_state->getLogLikelihood();
    }

    virtual ~MDL_CONTINUE_T_HYPO()
    {
        delete m_state;
    }

    /*-------------------------------------------------------------------*
     | MDL_CONTINUE_T_HYPO::makeDefaultChildren() -- make default
     |                                               children for a
     |                                               CONTINUE node
     |
     | This makes the children that are not linked to a report.
     *-------------------------------------------------------------------*/
    virtual void makeDefaultChildren()
    {
        MODEL *mdl = m_state->getMdl();
        double endLogLikelihood = mdl->getEndLogLikelihood( m_state );
        double continueLogLikelihood =
                mdl->getContinueLogLikelihood( m_state );
        double skipLogLikelihood =
                mdl->getSkipLogLikelihood( m_state );
        MDL_STATE *state;
        int numNewStates;
        int i;

        if( endLogLikelihood != -INFINITY )
            this->installChild( new MDL_END_T_HYPO<CORNER>(
                              this->m_mdlMht,
                              this->m_logLikelihood,
                              skipLogLikelihood,
                              endLogLikelihood ) );

        if( continueLogLikelihood != -INFINITY )
        {
            numNewStates = mdl->beginNewStates( m_state, 0 );

            for( i = 0; i < numNewStates; i++ )
            {
                state = mdl->getNewState( i, m_state, 0 );
                if( state != 0 )
                    this->installChild( new MDL_SKIP_T_HYPO<CORNER>(
                                      this->m_mdlMht,
                                      this->m_logLikelihood,
                                      continueLogLikelihood,
                                      skipLogLikelihood,
                                      state ) );
            }

            mdl->endNewStates();
        }
    }

    /*-------------------------------------------------------------------*
     | MDL_CONTINUE_T_HYPO::makeChildrenFor() -- make children for a
     |                                           CONTINUE node
     |
     | This makes the children that ARE linked to a report.
     *-------------------------------------------------------------------*/
    virtual void makeChildrenFor( MDL_REPORT *report )
    {

        MODEL *mdl = this->m_state->getMdl();
        double continueLogLikelihood =
                mdl->getContinueLogLikelihood( m_state );
        double detectLogLikelihood =
                mdl->getDetectLogLikelihood( m_state );
        MDL_STATE *state;
        int numNewStates;
        int i;

        numNewStates = mdl->beginNewStates( m_state, report );

        for( i = 0; i < numNewStates; i++ )
        {
            state = mdl->getNewState( i, m_state, report );
            if( state != 0 )
                this->installChild( new MDL_CONTINUE_T_HYPO<CORNER>(
                                  this->m_mdlMht,
                                  this->m_logLikelihood,
                                  continueLogLikelihood,
                                  detectLogLikelihood,
                                  state, report ) );
        }

        mdl->endNewStates();
    }
    virtual void verify()
    {
        this->m_mdlMht->continueTrack(
                    this->getTrackStamp(),
                    this->getTimeStamp(),
                    this->m_state,
                    (MDL_REPORT *)this->getReport() );
    }

public:

    virtual MDL_STATE *getState()
    {
        return m_state;
    }
    virtual void print()
    {
        std::cout << "T:" << this->getTrackStamp()<<"[";
        getState()->print();
        std::cout << " ," ;
        this->getReport()->print() ;
        std::cout << "](continue:"
                  << this->m_logLikelihood<<")";
    }
};

template<class CORNER>
class MDL_START_T_HYPO: public MDL_CONTINUE_T_HYPO<CORNER>
{
    friend class MDL_MHT<CORNER>;
    friend class MDL_ROOT_T_HYPO<CORNER>;
    friend class MDL_FALARM_T_HYPO<CORNER>;
    friend class MDL_DUMMY_T_HYPO<CORNER>;
    friend class MDL_CONTINUE_T_HYPO<CORNER>;
    friend class MDL_SKIP_T_HYPO<CORNER>;
    friend class MDL_END_T_HYPO<CORNER>;

protected:

    MDL_START_T_HYPO( MDL_MHT<CORNER> *mdlMht,
                      MDL_STATE *state, MDL_REPORT *report ):
        MDL_CONTINUE_T_HYPO<CORNER>( mdlMht, report )
    {
        this->m_state = state;
        this->m_endsTrack = 0;
        this->m_mustVerify = 1;
        this->m_logLikelihood = this->m_state->getLogLikelihood();
    }

    virtual void verify()
    {
        this->m_mdlMht->startTrack(
                    this->getTrackStamp(),
                    this->getTimeStamp(),
                    this->m_state,
                    (MDL_REPORT *)this->getReport() );
    }

public:

    virtual void print()
    {
        std::cout << "T:"<<this->getTrackStamp()<<"[";
        this->getReport()->print();
        std::cout << "](start:"<<this->m_logLikelihood<<")";
    }
};

template<class CORNER>
class MDL_SKIP_T_HYPO: public MDL_CONTINUE_T_HYPO<CORNER>
{
    friend class MDL_MHT<CORNER>;
    friend class MDL_ROOT_T_HYPO<CORNER>;
    friend class MDL_FALARM_T_HYPO<CORNER>;
    friend class MDL_DUMMY_T_HYPO<CORNER>;
    friend class MDL_START_T_HYPO<CORNER>;
    friend class MDL_CONTINUE_T_HYPO<CORNER>;
    friend class MDL_END_T_HYPO<CORNER>;

protected:

    MDL_SKIP_T_HYPO( MDL_MHT<CORNER> *mdlMht,
                     double trackLogLikelihood,
                     double continueLogLikelihood,
                     double skipLogLikelihood,
                     MDL_STATE *state ):
        MDL_CONTINUE_T_HYPO<CORNER>( mdlMht )
    {
        this->m_state = state;
        this->m_endsTrack = 0;
        this->m_mustVerify = 1;
        this->m_logLikelihood = trackLogLikelihood +
                          continueLogLikelihood +
                          skipLogLikelihood +
                          state->getLogLikelihood();
    }

    virtual void verify()
    {
        this->m_mdlMht->skipTrack(
                    this->getTrackStamp(),
                    this->getTimeStamp(),
                    this->m_state );
    }

public:

    virtual void print()
    {
        std::cout << "T:"<<this->getTrackStamp()<<"[";
        this->getState()->print();
        std::cout << "](skip:"<< this->m_logLikelihood<<")";
    }
};

template<class CORNER>
class MDL_END_T_HYPO: public MDL_DUMMY_T_HYPO<CORNER>
{
    friend class MDL_MHT<CORNER>;
    friend class MDL_ROOT_T_HYPO<CORNER>;
    friend class MDL_FALARM_T_HYPO<CORNER>;
    friend class MDL_DUMMY_T_HYPO<CORNER>;
    friend class MDL_START_T_HYPO<CORNER>;
    friend class MDL_CONTINUE_T_HYPO<CORNER>;
    friend class MDL_SKIP_T_HYPO<CORNER>;

protected:

    MDL_END_T_HYPO( MDL_MHT<CORNER> *mdlMht,
                    double trackLogLikelihood,
                    double skipLogLikelihood,
                    double endLogLikelihood ):
        MDL_DUMMY_T_HYPO<CORNER>( mdlMht )
    {
        this->m_endsTrack = 1;
        this->m_mustVerify = 1;
        this->m_logLikelihood = trackLogLikelihood +
                          skipLogLikelihood +
                          endLogLikelihood;
    }

    virtual void verify()
    {
        this->m_mdlMht->endTrack( this->getTrackStamp(), this->getTimeStamp() );
    }

public:

    virtual void print()
    {
        std::cout << "T: "
                  << this->getTrackStamp()
                  << "( "<<(void *)this
                  << ")"<<"(end:"
                  << this->m_logLikelihood<<")";
    }
};

#endif
