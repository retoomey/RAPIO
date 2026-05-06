#include "computeDR.h" //The local file
//#include <rRadialSet.h> //needed for any RadialSet objects you might send include
#include <rError.h> //Logging information uses this header
#include <rConstants.h> //Constant::MissingData 
#include <cmath> //for pow() and min
//this is always a good idea so that the compiler knows you are 
// part of the rapio environment.
namespace rapio {

namespace {
// This anonymous namespace is a location for values only 
//  used by this algorithm itself.
//
//  The maximum allowed value of CC. Because data >= 1.0 causes the equation
//  to become non-finite we limit cc to this value.
    const float ccMax = 0.9999;
//
// This anonymous namespace is good place to keep other algorithmic threshold values.
//
} //end of anonymous namespace

//Note how the science part is sperate from the data acquisition and
// even the walking of arrays. This allows others to use the science
// and computeDR elsewhere if needed. We try to keep the science
// as seperate from the data flow as possible.
//
float computeDR( float cc, float zdr, float MissingFlag) {
    //convert Zdr into linear units
    auto zdrlin = pow(10.0,zdr/10.0);
    if ( !finite(zdrlin)) {
        return MissingFlag;
    }
  
    //auto drVal = (1.0 + pow(zdrlin,-1.0) - 2.0*fmin(ccMax,cc)*(pow(zdrlin,-1.0/2.0))) / 
    //             (1.0 + pow(zdrlin,-1.0) + 2.0*fmin(ccMax,cc)*(pow(zdrlin,-1.0/2.0))) ;
    auto drVal = (1.0 + zdrlin - 2.0*fmin(ccMax,cc)*(pow(zdrlin,1.0/2.0))) / 
                 (1.0 + zdrlin + 2.0*fmin(ccMax,cc)*(pow(zdrlin,1.0/2.0))) ;

    if ( !finite(drVal)) {
        return MissingFlag;
    } else {
        return 10.0*log10(drVal);
    }

}

}//end of namespace rapio
