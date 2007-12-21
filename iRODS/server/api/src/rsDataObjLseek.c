/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See dataObjLseek.h for a description of this API call.*/

#include "dataObjLseek.h"
#include "rodsLog.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "subStructFileLseek.h"
#include "objMetaOpr.h"
#include "subStructFileUnlink.h"


int
rsDataObjLseek (rsComm_t *rsComm, fileLseekInp_t *dataObjLseekInp, 
fileLseekOut_t **dataObjLseekOut)
{
    int status;
    int l1descInx, l3descInx;
    int rescTypeInx;
    dataObjInfo_t *dataObjInfo;

    l1descInx = dataObjLseekInp->fileInx;

    if (l1descInx <= 2 || l1descInx >= NUM_L1_DESC) {
       rodsLog (LOG_NOTICE,
         "rsDataObjLseek: l1descInx %d out of range",
         l1descInx);
        return (SYS_FILE_DESC_OUT_OF_RANGE);
    }

    l3descInx = L1desc[l1descInx].l3descInx;

    if (l3descInx <= 2) {
       rodsLog (LOG_NOTICE,
         "rsDataObjLseek: l3descInx %d out of range",
         l3descInx);
        return (SYS_FILE_DESC_OUT_OF_RANGE);
    }

    dataObjInfo = L1desc[l1descInx].dataObjInfo;

    if (getBunType (dataObjInfo->specColl) >= 0) {
        bunSubLseekInp_t bunSubLseekInp;
        memset (&bunSubLseekInp, 0, sizeof (bunSubLseekInp));
	bunSubLseekInp.type = dataObjInfo->specColl->type;
        bunSubLseekInp.fd = L1desc[l1descInx].l3descInx;
	bunSubLseekInp.offset = dataObjLseekInp->offset;
	bunSubLseekInp.whence = dataObjLseekInp->whence;
        rstrcpy (bunSubLseekInp.addr.hostAddr, dataObjInfo->rescInfo->rescLoc,
          NAME_LEN);
        status = rsBunSubLseek (rsComm, &bunSubLseekInp, dataObjLseekOut);
    } else {
        *dataObjLseekOut = malloc (sizeof (fileLseekOut_t));
        memset (*dataObjLseekOut, 0, sizeof (fileLseekOut_t));
        rescTypeInx = dataObjInfo->rescInfo->rescTypeInx;

        (*dataObjLseekOut)->offset = _l3Lseek (rsComm, rescTypeInx, l3descInx,
          dataObjLseekInp->offset, dataObjLseekInp->whence); 

        if ((*dataObjLseekOut)->offset >= 0) {
	    status = 0;
        } else {
	    status = (*dataObjLseekOut)->offset;
        }
    }

    return (status);
}

rodsLong_t
_l3Lseek (rsComm_t *rsComm, int rescTypeInx, int l3descInx,
rodsLong_t offset, int whence)
{
    fileLseekInp_t fileLseekInp;
    fileLseekOut_t *fileLseekOut = NULL;
    int status;

    switch (RescTypeDef[rescTypeInx].rescCat) {
      case FILE_CAT:
        memset (&fileLseekInp, 0, sizeof (fileLseekInp));
        fileLseekInp.fileInx = l3descInx;
        fileLseekInp.offset = offset;
        fileLseekInp.whence = whence;
        status = rsFileLseek (rsComm, &fileLseekInp, &fileLseekOut);
        break;

      default:
        rodsLog (LOG_NOTICE,
          "_l3Lseek: rescCat type %d is not recognized",
          RescTypeDef[rescTypeInx].rescCat);
        status = SYS_INVALID_RESC_TYPE;
        break;
    }
    if (status < 0) {
        return (status);
    } else {
	rodsLong_t offset = fileLseekOut->offset;
	free (fileLseekOut);
        return (offset);
    }
}

