#include <cstdlib>
#include <cstdio>	// for sscanf
#include <string.h>
#include <cmath>
#include <iostream>	// for std::cout, std::endl, std::cerr
#include <fstream>	// for ifstream and ofstream
#include <sstream>	// for stringstream
#include <list>		// for std::list<>
#include <vector>	// for std::vector<>
#include "param.h"       //  contains values of needed parameters 
#include "motionModel.h"

#include <stdexcept>	// for std::runtime_error

#include <unistd.h>	// for optarg, opterr, optopt
#include <getopt.h>	// for getopt_long()



/*
 * External Variables
 */
//int g_isFirstScan=1;
//iDLIST_OF< FALARM > *g_falarms_ptr;   // list of false alarms found
//iDLIST_OF< CORNER_TRACK > *g_cornerTracks_ptr; // list of cornerTracks found
//CORNERLISTXY *g_currentCornerList;
int g_time;

void PrintSyntax()
{
    std::cerr << "trackCorners -o OUTFILE [-p PARAM_FILE] [-d DIRNAME] -i INFILE\n"
              << "             [--syntax | -x] [--help | -h]\n";
}

void PrintHelp()
{
    PrintSyntax();
    std::cerr << "-o  --output  OUTFILE\n"
              << "The file that you want to write the track data to.\n\n";

    std::cerr << "-p  --param   PARAM_FILE\n"
              << "The file where the tracking parameters can be found.\n"
              << "Defaults to './Parameters'.\n\n";

    std::cerr << "-i  --input   INFILE\n"
              << "INFILE contains the filestem of the corner files, the range of frames\n"
              << "and the number of identified features for each frame.\n\n";

    std::cerr << "-d --dir      DIRNAME\n"
              << "DIRNAME to prepend to the corner files.  Default is .\n\n";

    std::cerr << "-x  --syntax\n"
              << "Print the syntax for running this program.\n\n";

    std::cerr << "-h  --help\n"
              << "Print this help page.\n\n";
}



int main(int argc, char **argv)
{
    Parameter read_param(const std::string &paramFileName);
    void writeCornerTrackFile(const std::string &trackFileName,
                              const Parameter &param,
                              const std::list< CORNER_TRACK > &cornerTracks,
                              const std::list< FALARM > &falarms);
    std::list<CORNERLISTXY> readCorners(const std::string &inputFileName,
				      const std::string &dirName);

    std::list<CORNERLISTXY> inputData;
    ptrDLIST_OF<MODEL> mdl;

    std::string outputFileName = "";
    std::string paramFileName = "./Parameters";
    std::string inputFileName = "";
    std::string dirName = ".";

    int OptionIndex = 0;
    int OptionChar = 0;
    bool OptionError = false;
    opterr = 0;			// don't print out error messages, let this program do that.

    static struct option TheLongOptions[] =
    {
        {"output", 1, NULL, 'o'},
        {"param", 1, NULL, 'p'},
        {"input", 1, NULL, 'i'},
	{"dir", 1, NULL, 'd'},
        {"syntax", 0, NULL, 'x'},
        {"help", 0, NULL, 'h'},
        {0, 0, 0, 0}
    };

    while ((OptionChar = getopt_long(argc, argv, "o:p:i:d:xh", TheLongOptions, &OptionIndex)) != -1)
    {
        switch (OptionChar)
        {
        case 'o':
            outputFileName = optarg;
            break;
        case 'p':
            paramFileName = optarg;
            break;
        case 'i':
            inputFileName = optarg;
            break;
	case 'd':
	    dirName = optarg;
	    break;
        case 'x':
            PrintSyntax();
            return(1);
            break;
        case 'h':
            PrintHelp();
            return(1);
            break;
        case '?':
            std::cerr << "ERROR: Unknown arguement: -" << (char) optopt << std::endl;
            OptionError = true;
            break;
        case ':':
            std::cerr << "ERROR: Missing value for arguement: -" << (char) optopt << std::endl;
            OptionError = true;
            break;
        default:
            std::cerr << "ERROR: Programming error... Unaccounted option: -" << (char) OptionChar << std::endl;
            OptionError = true;
            break;
        }
    }

    if (OptionError)
    {
        PrintSyntax();
        return(-1);
    }

    if (outputFileName.empty())
    {
        std::cerr << "ERROR: Missing OUTFILE name\n";
        PrintSyntax();
        return(-1);
    }

    if (inputFileName.empty())
    {
        std::cerr << "ERROR: Missing INFILE name\n";
        PrintSyntax();
        return(-1);
    }

    if (paramFileName.empty())
    {
        std::cerr << "ERROR: Missing or empty PARAM_FILE name\n";
        PrintSyntax();
        return(-1);
    }

    if (dirName.empty())
    {
	std::cerr << "ERROR: Missing or empty DIRNAME\n";
        PrintSyntax();
        return(-1);
    }


    /*
     * Create the global list of CornerTracks & false alarms
     * Every corner will be assigned to to either a CornerTrack or a false alarm
     */

    //g_cornerTracks_ptr = new iDLIST_OF<CORNER_TRACK>;
    //g_falarms_ptr = new iDLIST_OF< FALARM >;


    /*
     * Read the parameters
     */

    const Parameter param = read_param(paramFileName.c_str());

    /*
     * Read the corners
     */

    inputData = readCorners(inputFileName, dirName);

    /*
     * Create constant velocity model
     */

    CONSTVEL_MDL *cvmdl = new CONSTVEL_MDL
    ( param.positionVarianceX,
      param.positionVarianceY,
      param.gradientVariance,
      param.intensityVariance,
      param.processVariance,
      param.meanNew,
      param.probEnd,
      param.probDetect,
      param.stateVariance,
      param.intensityThreshold,
      param.maxDistance2);
    mdl.append( (*cvmdl) );




    /*
     * Setup mht algorithm with CornerTrack model mdl
     */

    CORNER_TRACK_MHT mht( param.meanFalarms,
                          param.maxDepth,
                          param.minGHypoRatio,
                          param.maxGHypos,
                          mdl );



    /*
     *  Get the 1st set of measurements or the corners of the
     *  1st frame
     */

    //std::list<CORNERLISTXY>::iterator cornerListIter = inputData.begin();
    //g_currentCornerList = cornerPtr.get();


    /*
     * Now do all the work by calling mht.scan()
     * mht.scan() returns 0 when there are no more measurements to be processed
     * Otherwise it gets the next set of measurements and processes them.
     *
     * AddReport adds another report the mht's queue.
     */
    std::cout << "About to scan...\n";

    int didIscan=0;
    for ( std::list<CORNERLISTXY>::iterator cornerListIter = inputData.begin();
          cornerListIter != inputData.end();
          cornerListIter++ )
    {
        mht.addReports(*cornerListIter);
        didIscan = mht.scan();
        std::cout << "******************CURRENT_TIME=" << mht.getCurrentTime() << ' '
                  << "ENDTIME=" << param.endScan << "****************\n";

        g_time=mht.getCurrentTime();
        mht.printStats(2);
        if( mht.getCurrentTime() > param.endScan )
        {
            break;
        }

        if (didIscan)
        {
//        mht.describe();
        }

    }
//    mht.describe();
    std::cout << "\n CLEARING \n" << std::endl;
    mht.clear();
//    mht.describe();

    /*
     * Finished finding CORNER_TRACKs so write out result
     * And write out dataFile containing list of CORNER_TRACKs and
     * associated corners
     */
    writeCornerTrackFile(outputFileName, param,
                         mht.GetTracks(), mht.GetFalseAlarms());

    return(0);

}

/*----------------------------------------------------------*
 * read_param():  Read the parameter file
 * A line that begins with ; in the file is considered
 * a comment. So skip the comments.
 *----------------------------------------------------------*/

Parameter read_param(const std::string &paramFile)
{
    std::ifstream fp( paramFile.c_str(), std::ios_base::in );
    if (!fp.is_open())
    {
        throw std::runtime_error("Couldn't open parameter file: " + paramFile);
    }


    std::cout << "Using Parameter File: " << paramFile << std::endl;
    Parameter param;
    std::string buf;
    
    // read lines from fp, skipping lines with ';' at the beginning and empty lines.
    // Note, this won't handle "empty" lines that have whitespace characters, I think.
    while( std::getline(fp, buf) && buf.size() > 0 && buf[ 0 ] == ';' ) ;
    if( fp )
    {
        param.positionVarianceX = atof( buf.c_str() );
    }

    while( std::getline(fp, buf) && buf.size() > 0 && buf[ 0 ] == ';' ) ;
    if( fp )
    {
        param.positionVarianceY = atof( buf.c_str() );
    }

    while( std::getline(fp, buf) && buf.size() > 0 && buf[ 0 ] == ';' ) ;
    if( fp )
    {
        param.gradientVariance = atof( buf.c_str() );
    }

    while( std::getline(fp, buf) && buf.size() > 0 && buf[ 0 ] == ';' ) ;
    if( fp )
    {
        param.intensityVariance = atof( buf.c_str() );
    }

    while( std::getline(fp, buf) && buf.size() > 0 && buf[ 0 ] == ';' ) ;
    if( fp )
    {
        param.processVariance = atof( buf.c_str() );
    }

    while( std::getline(fp, buf) && buf.size() > 0 && buf[ 0 ] == ';' ) ;
    if( fp )
    {
        param.probDetect = atof( buf.c_str() );
    }

    while( std::getline(fp, buf) && buf.size() > 0 && buf[ 0 ] == ';' ) ;
    if( fp )
    {
        param.probEnd = atof( buf.c_str() );
    }

    while( std::getline(fp, buf) && buf.size() > 0 && buf[ 0 ] == ';' ) ;
    if( fp )
    {
        param.meanNew = atof( buf.c_str() );
    }

    while( std::getline(fp, buf) && buf.size() > 0 && buf[ 0 ] == ';' ) ;
    if( fp )
    {
        param.meanFalarms = atof( buf.c_str() );
    }

    while( std::getline(fp, buf) && buf.size() > 0 && buf[ 0 ] == ';' ) ;
    if( fp )
    {
        param.maxGHypos = atoi( buf.c_str() );
    }

    while( std::getline(fp, buf) && buf.size() > 0 && buf[ 0 ] == ';' ) ;
    if( fp )
    {
        param.maxDepth = atoi( buf.c_str() );
    }

    while( std::getline(fp, buf) && buf.size() > 0 && buf[ 0 ] == ';' ) ;
    if( fp )
    {
        param.minGHypoRatio = atof( buf.c_str() );
    }

    while( std::getline(fp, buf) && buf.size() > 0 && buf[ 0 ] == ';' ) ;
    if( fp )
    {
        param.intensityThreshold = atof( buf.c_str() );
    }

    while( std::getline(fp, buf) && buf.size() > 0 && buf[ 0 ] == ';' ) ;
    if( fp )
    {
        param.maxDistance1 = atof( buf.c_str() );
    }

    while( std::getline(fp, buf) && buf.size() > 0 && buf[ 0 ] == ';' ) ;
    if( fp )
    {
        param.maxDistance2 = atof( buf.c_str() );
    }

    while( std::getline(fp, buf) && buf.size() > 0 && buf[ 0 ] == ';' ) ;
    if( fp )
    {
        param.maxDistance3 = atof( buf.c_str() );
    }

    while( std::getline(fp, buf) && buf.size() > 0 && buf[ 0 ] == ';' ) ;
    if( fp )
    {
        param.stateVariance = atof( buf.c_str() );
    }

    while( std::getline(fp, buf) && buf.size() > 0 && buf[ 0 ] == ';' ) ;
    if( fp )
    {
        param.endScan = atoi( buf.c_str() );
    }

    while( std::getline(fp, buf) && buf.size() > 0 && buf[ 0 ] == ';' ) ;
    if( fp )
    {
        param.pos2velLikelihood = atoi( buf.c_str() );
    }

    while( std::getline(fp, buf) && buf.size() > 0 && buf[ 0 ] == ';' ) ;
    if( fp )
    {
        param.vel2curvLikelihood = atoi( buf.c_str() );
    }

    while( std::getline(fp, buf) && buf.size() > 0 && buf[ 0 ] == ';' ) ;
    if( fp )
    {
        param.startA = atoi( buf.c_str() );
    }

    while( std::getline(fp, buf) && buf.size() > 0 && buf[ 0 ] == ';' ) ;
    if( fp )
    {
        param.startB = atoi( buf.c_str() );
    }

    while( std::getline(fp, buf) && buf.size() > 0 && buf[ 0 ] == ';' ) ;
    if( fp )
    {
        param.startC = atoi( buf.c_str() );
    }


    fp.close();


    std::cout << " positionVarianceX = " << param.positionVarianceX << std::endl;
    std::cout << " positionVarianceY = " << param.positionVarianceY << std::endl;
    std::cout << " gradientVariance = " << param.gradientVariance << std::endl;
    std::cout << " intensityVariance = " << param.intensityVariance << std::endl;
    std::cout << " processVariance = " << param.processVariance << std::endl;
    std::cout << " probDetect = " << param.probDetect << std::endl;
    std::cout << " probEnd = " << param.probEnd << std::endl;
    std::cout << " meanNew = " << param.meanNew << std::endl;
    std::cout << " meanFalarms = " << param.meanFalarms << std::endl;
    std::cout << " maxGHypos = " << param.maxGHypos << std::endl;
    std::cout << " maxDepth = " << param.maxDepth << std::endl;
    std::cout << " minGHypoRatio = " << param.minGHypoRatio << std::endl;
    std::cout << " intensityThreshold= " << param.intensityThreshold << std::endl;
    std::cout << " maxDistance1= " << param.maxDistance1 << std::endl;
    std::cout << " maxDistance2= " << param.maxDistance2 << std::endl;
    std::cout << " maxDistance3= " << param.maxDistance3 << std::endl;

    return param;
}

/*----------------------------------------------------------*
 * writeCornerTrackFile():  Write all information regarding
 * the CORNER_TRACKs into a file.
 *----------------------------------------------------------*/

void writeCornerTrackFile(const std::string &name, const Parameter &param,
                          const std::list< CORNER_TRACK > &cornerTracks,
                          const std::list< FALARM > &falarms)
{
    int id;

    std::ofstream CornerTrackFile(name.c_str(), std::ios_base::out);

    if (!CornerTrackFile.is_open())
    {
        throw std::runtime_error("Could not open corner track file: " + name);
    }

    CornerTrackFile << "#INFORMATION REGARDING THIS CORNER TRACKER\n"
                    << "#___________________________________________\n"
                    << "#\n#\n"
                    /*
                     * Write out the parameters that were used
                     */
                    << "#    Parameters: \n"
                    << "#\n"
                    << "#         PositionVarianceX:  " << param.positionVarianceX << "\n"
                    << "#\n"
                    << "#         PositionVarianceY:  " << param.positionVarianceY << "\n"
                    << "#\n"
                    << "#         GradientVariance:  " << param.gradientVariance << "\n"
                    << "#\n"
                    << "#         intensityVariance:  " << param.intensityVariance << "\n"
                    << "#\n"
                    << "#         ProcessVariance:  " << param.processVariance << "\n"
                    << "#\n"
                    << "#         StateVariance:  " << param.stateVariance << "\n"
                    << "#\n"
                    << "#         Prob. Of Detection:  " << param.probDetect << "\n"
                    << "#\n"
                    << "#         Prob Of Track Ending:  " << param.probEnd << "\n"
                    << "#\n"
                    << "#         Mean New Tracks:  " << param.meanNew << "\n"
                    << "#\n"
                    << "#         Mean False Alarms:  " << param.meanFalarms << "\n"
                    << "#\n"
                    << "#         Max Global Hypo:  " << param.maxGHypos << "\n"
                    << "#\n"
                    << "#         Max Depth:  " << param.maxDepth << "\n"
                    << "#\n"
                    << "#         MinGHypoRatio:  " << param.minGHypoRatio << "\n"
                    << "#\n"
                    << "#         intensity Threshold:  " << param.intensityThreshold << "\n"
                    << "#\n"
                    << "#         Max Mahalinobus Dist1:  " << param.maxDistance1 << "\n"
                    << "#\n"
                    << "#         Max Mahalinobus Dist2:  " << param.maxDistance1 << "\n"
                    << "#\n"
                    << "#         Max Mahalinobus Dist3:  " << param.maxDistance1 << "\n"
                    << "#" << std::endl;

    /*
     * Write the number of CornerTracks & falsealarms
     */
    CornerTrackFile << cornerTracks.size() << "\n"
                    << falarms.size() << std::endl;

    /*
     * Information about each CornerTrack is organized as follows:
     *     CornerTrackId  CornerTrackLength CornerTrackColor
     *     Code Measurement(x dx y dy)  EstimatedState(x dx y dy)
     *
     *     The Code is either M or S. M, if there was a mesurement
     *     and 'S'(skipped) if no measurement was found at that
     *      time step
     */
    // Looping over each track
    id = 0;
    for (std::list<CORNER_TRACK>::const_iterator cornerTrack = cornerTracks.begin();
         cornerTrack != cornerTracks.end();
         cornerTrack++)
    {
        CornerTrackFile << id++ << ' ' << cornerTrack->list.size() << std::endl;

        // Looping over each corner of the track
        for( std::list<CORNER_TRACK_ELEMENT>::const_iterator cornerTrackEl = cornerTrack->list.begin();
             cornerTrackEl != cornerTrack->list.end();
             cornerTrackEl++ )
        {
            CornerTrackFile << (cornerTrackEl->hasReport ? 'M':'S') << ' '
                            << cornerTrackEl->rx << ' '
                            << cornerTrackEl->ry << ' '
                            << cornerTrackEl->sx << ' '
                            << cornerTrackEl->sy << ' '
                            << cornerTrackEl->logLikelihood << ' '
                            << cornerTrackEl->time << ' '
                            << cornerTrackEl->frameNo << ' '
                            << cornerTrackEl->model << ' '
                            << cornerTrackEl->cornerID << std::endl;
        }
    }

    /*
     * Information about each false alarm
     *     x y t
     */
    // Looping over the false alarms
    for (std::list<FALARM>::const_iterator falarm = falarms.begin();
         falarm != falarms.end();
         falarm++)
    {
        CornerTrackFile << falarm->rX << ' '
                        << falarm->rY << ' '
                        << falarm->frameNo << ' '
                        << falarm->cornerID << std::endl;
    }

    CornerTrackFile.close();
}

/*------------------------------------------------------------------*
 * readCorners():  Read corners from the motion sequence.  Currently
 * information about corner files, their name, start frame end frame
 * and the number of corners in each frame are read from stdin
 *
 *------------------------------------------------------------------*/


std::list<CORNERLISTXY> readCorners(const std::string &inputFileName, const std::string &dirName)
// TODO: Make this platform-independent by dynamically choosing the correct path separator.
//       Right now, it assumes Unix-based paths
// NOTE: Supposedly, using forward-slashes should work for Windows.
//       I don't have a Windows machine to test this, though.
{
    std::list<CORNERLISTXY> inputData;
    std::vector<int> ncorners(0);

    std::string str;
    int npoints;
    // TODO: There is probably a very smart reason for this, but it escapes me at the moment
    //       I suspect that it has something to do with a case where the control file is empty
    //       maybe?
    int startFrame=4;
    int totalFrames;
    std::string basename;
    std::ifstream controlFile(inputFileName.c_str(), std::ios_base::in);
    if (!controlFile.is_open())
    {
        throw std::runtime_error("Could not open the input data file: " + inputFileName + "\n");
    }

    /*
     * Read basename for image sequence, total # of  frames, start
     * frame number, and number of corners in each frame
     */

    controlFile >> basename >> totalFrames >> startFrame;
    // There can be additional optional pieces of data.
    // Therefore, we grab the rest of the line, and load it into
    // a stringstream so that we can load those value(s) separately.
    // The reason for the awkward approach is because we don't want
    // to grab a value on the next line.
    std::string optionalData;
    std::getline(controlFile, optionalData);

    std::stringstream optionStrm(optionalData);

    float timeDelta = 1.;
    optionStrm >> timeDelta;

    // Now, find out how many features are in each frame.
    for (int frameIndex=0; frameIndex < totalFrames; frameIndex++)
    {
        controlFile >> npoints;
        ncorners.push_back(npoints);
        std::cout << "ncorners[" << frameIndex << "]=" << ncorners[frameIndex] << std::endl;
        inputData.push_back(CORNERLISTXY(npoints, timeDelta));
    }

    controlFile.close();

    //size_t cornerID = 0;
    /*
     * Open each frame and read the corner Data from them, saving
     * the data in inputData
     */
    int i = startFrame;
    for (std::list<CORNERLISTXY>::iterator aCornerList = inputData.begin();
         aCornerList != inputData.end();
         aCornerList++)
    {
        float x,y;
        float i1,i2,i3,i4,i5,i6,i7,i8,i9,i10,i11,i12,i13,i14,i15;
        float i16,i17,i18,i19,i20,i21,i22,i23,i24,i25;
        size_t cornerID;
        std::stringstream stringRep;
        stringRep << basename << '.' << i++;
	// NOTE: this is where I lose platform-independence by statically using '/'
        std::string fname = dirName + "/" + stringRep.str();
        std::ifstream inDataFile(fname.c_str(), std::ios_base::in);
//    std::cout << "Reading file " << fname << "\n";

        if (!inDataFile.is_open())
        {
            throw std::runtime_error("Could not open the input data file: " + fname);
        }

        int j=0;
        while (std::getline(inDataFile, str) && j < ncorners[i-startFrame-1])
        {
            sscanf(str.c_str(),"%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %zd",
                   &x,&y,&i1,&i2,&i3,&i4,&i5,&i6,&i7,&i8, &i9, &i10, &i11, &i12, &i13, &i14,
                   &i15,&i16, &i17, &i18, &i19, &i20, &i21, &i22, &i23, &i24, &i25, &cornerID );

            aCornerList->list.push_back(CORNERXY(x,y, Texture_t(i1,i2,i3,i4,i5,i6,i7,i8,i9,i10,i11,i12,i13,i14,i15,i16,i17,i18,i19,i20,i21,i22,i23,i24,i25),i-1,cornerID));
            j++;
//            cornerID++;
        }
        inDataFile.close();
    }

    return inputData;

}

