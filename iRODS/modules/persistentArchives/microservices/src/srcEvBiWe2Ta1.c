int intChkDataType  (rsComm_t * rsComm, char *strFullDataPath, time_t tTime, ruleExecInfo_t * rei);
int intGetDataObjACL(dataObjInp_t *myDataObjInp, bytesBuf_t *mybuf, rsComm_t *rsComm);
int rsObjStat (rsComm_t *rsComm, dataObjInp_t *dataObjInp, rodsObjStat_t **rodsObjStatOut);

#include "genQuery.h"
#include "modAVUMetadata.h"
int rsGenQuery (rsComm_t *rsComm, genQueryInp_t *genQueryInp, genQueryOut_t **genQueryOut);
int rsModAVUMetadata (rsComm_t *rsComm, modAVUMetadataInp_t *modAVUMetadataInp );

/*
 * \fn msiChkRechkRecompChkSum4DatObjVol2
 * \author  Sifang Lu
 * \date   2007-10-01
 * \brief This microservice iterate through collection, and calls 
 *  rsDataObjRepl to recursively replication a collection
 *  as part of a workflow  execution.
 * \note This call should only be used through the rcExecMyRule (irule) call
 *  i.e., rule execution initiated by clients and should not be called
 *  internally by the server since it interacts with the client through
 *  the normal client/server socket connection.
 * \param[in]
 *    coll     : It can be a collInp_t or a STR_MS_T which would be taken 
 *               as destination collection path.
 *    destResc : STR_MS_T destination resource name
 *    options  : STR_MS_T a group of options in a string delimited by '%%'.
 *               If the string is empty ("\0") or null ("NULL") it will not 
 *               be used.  
 *               The options can be the following
 *              - "all"(ALL_KW) 
 *              - "irodsAdmin" (IRODS_ADMIN_KW).
 *              - "backupMode" if specified, it will try to use 'backup mode' 
 *                to the destination resource. Means if a good copy already
 *                exists in destination resource, it will not throw an error
 *
 * \param[out] a INT_MS_T containing the status.
 * \return integer
 * \retval 0 on success
 * \sa
 * \post
 * \pre
 * \bug  no known bugs
**/

/*
  msiChkRechkRecompChkSum4DatObjVol2 (msParam_t *coll, msParam_t *destRescName, msParam_t *options,  msParam_t *outParam, ruleExecInfo_t *rei)
  msiChkRechkRecompChkSum4DatObj222 (msParam_t * inpParam1, msParam_t * inpParam2, msParam_t * outParam1, ruleExecInfo_t * rei)
*/

int
msiChkDataType (msParam_t *coll, msParam_t * inpParam2, msParam_t * inpParam3, msParam_t *outParam, ruleExecInfo_t *rei)
{
    rsComm_t *rsComm; 
    collInp_t collInp, *myCollInp;
    int iErr=0, i, continueInx, status;
    transStat_t *transStat = NULL;
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    dataObjInp_t dataObjInp; 
    time_t t1;
    char strOut[MAX_NAME_LEN * MAX_NAME_LEN], *strTimeDiff, myGlbPar1[MAX_NAME_LEN]="", *strDataTypeInput1;
    long lMax=100;
    int iCountMin = 0, iCountMax = 0, iCountMid = 0;

bytesBuf_t *mybuf;
 
    RE_TEST_MACRO ("    Calling msiChkDataType")

    if (rei == NULL || rei->rsComm == NULL) {
	    rodsLog (LOG_ERROR,
	    "msiChkDataType: input rei or rsComm is NULL");
	    return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    rsComm = rei->rsComm;

    /* parse inpParam1: coll */
    rei->status = parseMspForCollInp (coll, &collInp, 
      &myCollInp, 0);
    if (rei->status < 0) {
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
          "msiChkDataType: input inpParam1 error. status = %d", rei->status);
	sprintf (strOut,
                 "usage 'foo iRods_collection size_min size_max', ERROR:  msiChkDataType(), input inpParam1 error\n");
        rodsLog (LOG_ERROR,
		 "msiChkDataType(),  input inpParam1 error.");
        i = fillStrInMsParam (outParam, strOut);        /* MsParam.c parse  addformatedtrsing to bytes WriteBytesBuff printMsParam.c */
  
        return (rei->status);
    }

  /* parse inpParam2 MinSize*/
    if ((strDataTypeInput1 = parseMspForStr (inpParam2)) != NULL)
      {
        iCountMin = iCountMin;
      }
    else
      {
	sprintf (strOut,
		 "usage 'foo iRods_collection size_min size_max', ERROR:  msiChkDataType(), input inpParam2 error\n");
	rodsLog (LOG_ERROR,
	       "msiChkDataType(),  input inpParam2 error.");
	i = fillStrInMsParam (outParam, strOut);	/* MsParam.c parse  addformatedtrsing to bytes WriteBytesBuff printMsParam.c */
	return (-1);
      }
    
  /* parse inpParam3 MaxSize*/
    if ((strTimeDiff = parseMspForStr (inpParam3)) != NULL)
      {
	lMax = strtol (strTimeDiff, (char **) NULL, 10);
      }
    else
      {
	sprintf (strOut,
		 "usage 'foo iRods_collection size_min size_max', ERROR:  msiChkDataType(), input inpParam3 error\n");
	rodsLog (LOG_ERROR,
	       "msiChkDataType(),  input inpParam3 error.");
	i = fillStrInMsParam (outParam, strOut);	/* MsParam.c parse  addformatedtrsing to bytes WriteBytesBuff printMsParam.c */
	return (-1);
      }
    

    /* (void) intChkDataType (rsComm, "/tempZone/home/rods/CVS/Entries.Log", (time_t) i, rei);        test blbost sobota */

/* iterate through all files */
    memset (&genQueryInp, 0, sizeof (genQueryInp));
    memset (&genQueryOut, 0, sizeof (genQueryOut));

    addInxIval (&genQueryInp.selectInp, COL_COLL_NAME, 1);
    addInxIval (&genQueryInp.selectInp, COL_DATA_NAME, 1);
    addInxIval (&genQueryInp.selectInp, COL_DATA_SIZE, 1);
    
    addInxIval (&genQueryInp.selectInp, COL_D_DATA_CHECKSUM, 1);
    addInxIval (&genQueryInp.selectInp, COL_D_MODIFY_TIME, 1);
    addInxIval (&genQueryInp.selectInp, COL_D_CREATE_TIME, 1);
    
    addInxIval (&genQueryInp.selectInp, COL_COLL_TYPE,  1); 
    /* ?? addInxIval (&genQueryInp->selectInp, COL_COLL_TYPE, 1); */
    addInxIval (&genQueryInp.selectInp, COL_DATA_SIZE, 1); /* #gjk2 file size in range
							      #gjk3 ACL user-uthorization pairs
							      #gjk4 ACL contain at least
							      #gjk5 does not ACL
							      #gjk6 AVU contain exactly
							      #gjk7 AVU only one
							      #gjk8 AVU does not have duplicates
							   */

    status = rsQueryDataObjInCollReCur (rsComm, myCollInp->collName, 
      &genQueryInp, &genQueryOut, NULL, 1);

    /* printf("GJK- begin 0001.1.1 in msiChkDataType(), status=(%d), myCollInp->collName=(%s)\n", status, myCollInp->collName); */
printf("GJK- begin 0001.0.1 status=(%d), myCollInp->collName=(%s)\n", status, myCollInp->collName);

printf("GJKa2a myGlbPar1=(%s), myCollInp->collName=(%s)\n", myGlbPar1, myCollInp->collName);
rstrcpy(myGlbPar1, myCollInp->collName, MAX_NAME_LEN);
printf("GJKa2b myGlbPar1=(%s), myCollInp->collName=(%s)\n", myGlbPar1, myCollInp->collName);

    if (status < 0 && status != CAT_NO_ROWS_FOUND) {
    	rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
    	  "msiChkDataType: msiChkDataType error for %s, stat=%d",
    	  myCollInp->collName, status);
    	rei->status=status;
      return (rei->status);
    }

    while (rei->status >= 0) {
      sqlResult_t *subColl, *dataObj, *sqlDatSize;

      /* get sub coll paths in the batch */
      subColl = getSqlResultByInx (genQueryOut, COL_COLL_NAME);
      dataObj = getSqlResultByInx (genQueryOut, COL_DATA_NAME);
      /* COL_DATA_SIZE */
      /*sqlDatSize = getSqlResultByInx (genQueryOut, COL_D_CREATE_TIME); */
      sqlDatSize = getSqlResultByInx (genQueryOut, COL_COLL_NAME);
      /* sqlDatSize = getSqlResultByInx (genQueryOut, COL_DATA_SIZE); */

      if (sqlDatSize == NULL) {
	printf ("GJK-P P.003.2.2 ERROR sqlDatSize == NULL, dataObjInp.objPath=(%s), genQueryOut->rowCnt=(%d), i=(%d)\n", dataObjInp.objPath, genQueryOut->rowCnt, i);
      }
      else {
	printf ("GJK-P P.003.2.2 OK sqlDatSize != NULL, dataObjInp.objPath=(%s), genQueryOut->rowCnt=(%d), i=(%d)\n", dataObjInp.objPath, genQueryOut->rowCnt, i);
      }
      
      if (subColl == NULL) {
	printf ("GJK-P P.004.2.2 ERROR subColl == NULL, dataObjInp.objPath=(%s), genQueryOut->rowCnt=(%d), i=(%d)\n", dataObjInp.objPath, genQueryOut->rowCnt, i);
	/*	if (dataObj == NULL && status < 0) { */
        if (dataObj == NULL) {
	  printf ("GJK-P P.004.2.3 ERROR dataObj == NULL, dataObjInp.objPath=(%s), genQueryOut->rowCnt=(%d), i=(%d)\n", dataObjInp.objPath, genQueryOut->rowCnt, i);
	  /* rei->status = rsGenQuery (rsComm, &genQueryInp, &genQueryOut); */
	  
	  rodsLog (LOG_ERROR,
		   "msiChkDataType: msiChkDataType failed, (%s) is not an iRods data object (dataObj == NULL), istatus=%d, rei->status=%d", myCollInp->collName, status, rei->status);
	  rei->status=UNMATCHED_KEY_OR_INDEX;
	  return (rei->status);
	}
	else {
	  /* delej single object */
	  iErr = intChkDataType (rsComm, dataObjInp.objPath, t1, rei);	/* test blbost sobota */
	  /* printf("GJK-P P.4001.0.2. in msiChkDataType(), dataObjInp.objPath=(%s), i=%d\n", dataObjInp.objPath, i); */
	  /* GJK return(0); */
	}
	rodsLog (LOG_ERROR,
		 "msiChkDataType: msiChkDataType failed, (%s) is not an iRods collection, rei->status=%d", myCollInp->collName, rei->status);
	rei->status=UNMATCHED_KEY_OR_INDEX;  
	return (rei->status);
      }
      /* get data names in the batch */
      if ((dataObj = getSqlResultByInx (genQueryOut, COL_DATA_NAME))
          == NULL) {
            rodsLog (LOG_ERROR, 
              "msiChkDataType: msiChkDataType for COL_DATA_NAME failed");
            rei->status=UNMATCHED_KEY_OR_INDEX;   
            return (rei->status);
      }
      
      for (i = 0; i < genQueryOut->rowCnt; i++) {
        char *tmpSubColl, *tmpDataName, *tmpDataSize;
	
	/*	./lib/api/include/objStat.h:__rsObjStat (rsComm_t *rsComm, dataObjInp_t *dataObjInp, int interFlag, */

	/* printf ("GJK-P P.002.2.2a  muj hlavni cyklus ! in msiChkDataType(), dataObjInp.objPath=(%s), genQueryOut->rowCnt=(%d), i=(%d)\n", dataObjInp.objPath, genQueryOut->rowCnt, i); */
	/* printf ("GJK-P P.002.2.2a dataObjInp.objPath=(%s), genQueryOut->rowCnt=(%d), i=(%d)\n", dataObjInp.objPath, genQueryOut->rowCnt, i); */
	
        tmpSubColl = &subColl->value[subColl->len * i];
        tmpDataName = &dataObj->value[dataObj->len * i];
        tmpDataSize = &sqlDatSize->value[sqlDatSize->len * i];

snprintf (dataObjInp.objPath, MAX_NAME_LEN, "%s/%s",
              tmpSubColl, tmpDataName);
 
    {
    rodsObjStat_t *rodsObjStatOut;
    int status7;
    genQueryInp_t genQueryInp7;    
    dataObjInp_t myDataObjInp7;


    dataObjInfo_t *dataObjInfoHead = NULL;
    char *accessPerm;

    /* check for valid connection */
    if (rsComm == NULL) {
        rodsLog (LOG_ERROR, "msiChkDataType(): input rsComm is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }
    
    memset (&genQueryInp7, 0, sizeof (genQueryInp_t));
    memset (&myDataObjInp7, 0, sizeof (dataObjInp_t));
    rstrcpy(myDataObjInp7.objPath, dataObjInp.objPath, MAX_NAME_LEN);  /* rstrcpy(destination, source, max_len) */
    /* memset (&rodsObjStatOut, 0, sizeof (rodsObjStat_t));
       status = rsObjStat(rsComm, myDataObjInp, &rodsObjStatOut);
       
       if data_type only
    */
    status7 = rsObjStat(rsComm, &myDataObjInp7, &rodsObjStatOut);
    /*
      status7 = rsObjStat(rsComm, &dataObjInp, &rodsObjStatOut);
      
      if (status7 == 0  && status7 == 1) {
      if ((long)rodsObjStatOut->objSize <= lMin) iCountMin++;
      if ((long)rodsObjStatOut->objSize >= lMax) iCountMax++;
      if ((long)rodsObjStatOut->objSize > lMin && (long)rodsObjStatOut->objSize < lMax) iCountMid++;
      
      }
      printf ("GJK-P P.007.2.2c dataObjInp.objPath=(%s), size=(%ld), createTime=(%s), status7=(%d)\n", dataObjInp.objPath, (long)rodsObjStatOut->objSize, rodsObjStatOut->createTime, status7);
    */
    
    /* 2008.02.11.
      typedef struct DataObjInfo {
      char objPath[MAX_NAME_LEN];
      char rescName[MAX_NAME_LEN];       / * This could be resource group * /
      char rescGroupName[MAX_NAME_LEN];       / * This could be resource group * /
      char dataType[MAX_NAME_LEN];
      rodsLong_t dataSize;
      char chksum[MAX_NAME_LEN];
      char version[MAX_NAME_LEN];
      char filePath[MAX_NAME_LEN];
      rescInfo_t *rescInfo;
      char dataOwnerName[MAX_NAME_LEN];
      char dataOwnerZone[MAX_NAME_LEN];
      int  replNum;
      int  replStatus;     / * isDirty flag * /
      char statusString[MAX_NAME_LEN];
      rodsLong_t  dataId;
      rodsLong_t  collId;
      int  dataMapId;
      char dataComments[LONG_NAME_LEN];
      char dataExpiry[TIME_LEN];
      char dataCreate[TIME_LEN];
      char dataModify[TIME_LEN];
      char dataAccess[MAX_NAME_LEN];
      int  dataAccessInx;
      char destRescName[MAX_NAME_LEN];
      char backupRescName[MAX_NAME_LEN];
      char subPath[MAX_NAME_LEN];
      specColl_t *specColl;
      struct DataObjInfo *next;
    } dataObjInfo_t;
*/

if (1 == 1) {
  status = getDataObjInfo (rsComm, &myDataObjInp7, &dataObjInfoHead, accessPerm, 1);

 if (status < 0) {
   rodsLog (LOG_ERROR,
	    "rsDataObjTrim: getDataObjInfo for (%s), status=%d", myDataObjInp7.objPath, status);
   return (status);
 }
 else {
   printf("GJK getDataObjInfo(), OK, for (%s), type=(%s), status=%d\n", myDataObjInp7.objPath, dataObjInfoHead->dataType, status);
   
   if (strcmp(strDataTypeInput1, dataObjInfoHead->dataType) == 0) {
     iCountMin++;
   }
   else {
     iCountMid++;
   }
   iCountMax++;
   
   freeAllDataObjInfo (dataObjInfoHead);

 }
}

 if (2 == 3) {
   status = intGetDataObjACL(&myDataObjInp7, mybuf, rsComm);
   printf ("GJK intGetDataObjACL, mybuf=(%s), status=%d\n", (char *)mybuf, status);
 }
 
    }

    /*
      typedef struct DataObjInp {
      char objPath[MAX_NAME_LEN];
      int createMode;
      int openFlags;      / * used for specCollInx in rcQuerySpecColl * /
      rodsLong_t offset;
      rodsLong_t dataSize;
      int numThreads;
      int oprType;
      specColl_t *specColl;
      keyValPair_t condInput;   / * include chksum flag and value * /
      } dataObjInp_t;
    */
    /*
      typedef struct rodsObjStat {
      rodsLong_t          objSize;        / * file size * /
      objType_t           objType;        / * DATA_OBJ_T or COLL_OBJ_T * /
      int                 numCopies;
      char                dataId[MAX_NAME_LEN];
      char                chksum[MAX_NAME_LEN];
      char                ownerName[MAX_NAME_LEN];
      char                ownerZone[MAX_NAME_LEN];
      char                createTime[TIME_LEN];
      char                modifyTime[TIME_LEN];
      specColl_t          *specColl;
    } rodsObjStat_t;
*/
    
    /* printf ("GJK-P P.002.2.2b  muj hlavni cyklus ! in msiChkDataType(), dataObjInp.objPath=(%s), genQueryOut->rowCnt=(%d), i=(%d)\n", dataObjInp.objPath, genQueryOut->rowCnt, i); */
    /* printf ("GJK-P P.002.2.2b dataObjInp.objPath=(%s), genQueryOut->rowCnt=(%d), i=(%d), tmpDataSize=(%s)\n", dataObjInp.objPath, genQueryOut->rowCnt, i, tmpDataSize); */
    
     if (1 != genQueryOut->rowCnt)
        {
          /* printf            ("GJK-P P.994.4.4. in intChkDataType(), rei->status=(%d), genQueryOut->rowCnt=(%d), (dataObjInp.objPath=(%s)\n",             rei->status, genQueryOut->rowCnt, dataObjInp.objPath);
	     return(-1); not enough lines);
	  */
        }

     /* GJK  rei->status = rsDataObjRepl (rsComm, &dataObjInp, &transStat); */
      rei->status = 0; /* GJK */
        if (rei->status<0)
        {
          rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
            "msiChkDataType: rsDataObjRepl failed %s, status = %d",
			    (&dataObjInp)->objPath,
          rei->status);
        }
	else {
	  /* GJK fake 2 iErr = intChkDataType (rsComm, dataObjInp.objPath, t1, rei);	test blbost sobota */
	  /* printf("GJK-P P.004001.0.1. in msiChkDataType(), dataObjInp.objPath=(%s), i=%d\n", dataObjInp.objPath, i); */
	}
        if (transStat != NULL) {
    	    free (transStat);
        }
      }
      
      continueInx = genQueryOut->continueInx;
      freeGenQueryOut (&genQueryOut);
      if (continueInx > 0) {
        /* More to come */
        genQueryInp.continueInx = continueInx;
        rei->status =  rsGenQuery (rsComm, &genQueryInp, &genQueryOut);
      } else {
        break;
      }
    }
    
    clearKeyVal (&dataObjInp.condInput);

    (void) snprintf (strOut, 255,
		     "%d data objects are of data type (%d data objects are not of data type) '%s' from total of %d data objects in the input '%s' iRods collection\n", iCountMin, iCountMax - iCountMin, strDataTypeInput1, iCountMax, myGlbPar1);
    i = fillStrInMsParam (outParam, strOut);     /* MsParam.c parse  add formated string to bytes WriteBytesBuff printMsParam.c */
    printf("GJK end strOut=(%s)\n", strOut);
    
    return (rei->status);
} /* msiChkDataType */

/*
 
Input : iRods absulute path of an object or collection,
Unix time in seconds for the time , if the file wa not checket after that input time, than check it and recompute the sum
and updata the AVY time stamp
*/

int
msiChkDataType222 (msParam_t * inpParam1, msParam_t * inpParam2, msParam_t * outParam1, ruleExecInfo_t * rei)
{
  rsComm_t *rsComm;
  collInp_t collInpCache, *ptrInpColl;
  int iErr = 0, i = 0;
  int iCountUserDefinedMetadata = 0;
  char strOut[MAX_NAME_LEN * MAX_NAME_LEN], *strTimeDiff;
  long lTmp = 0;
  time_t t1;

  /* For testing mode when used with irule --test */
  RE_TEST_MACRO ("RE_TEST_MACRO, begin of msiChkDataType");

  printf ("GJK-P P.2222.0.1. in msiChkDataType()\n");

  rsComm = rei->rsComm;

  /* parse inpParam1 */
  rei->status = parseMspForCollInp (inpParam1, &collInpCache, &ptrInpColl, 0);

  if (rei->status < 0)
    {
      rodsLog (LOG_ERROR,
	       "msiChkDataType(),  input inpParam1 error. status = %d",
	       rei->status);
      return (rei->status);
    }

  /* parse inpParam2 */
  if ((strTimeDiff = parseMspForStr (inpParam2)) != NULL)
    {
      lTmp = strtol (strTimeDiff, (char **) NULL, 10);
      t1 = (time_t) lTmp;
    }
  else
    {
      sprintf (strOut,
	       "ERROR:  msiChkDataType(), input inpParam2 error\n");
      rodsLog (LOG_ERROR,
	       "msiChkDataType(),  input inpParam2 error.");
      i = fillStrInMsParam (outParam1, strOut);	/* MsParam.c parse  addformatedtrsing to bytes WriteBytesBuff printMsParam.c */
      return (-1);
    }

  printf
    ("GJK-P P.2222.0.2. in msiChkDataType(), ptrInpColl->collName=(%s), t1=%ld\n",
     ptrInpColl->collName, t1);

  iErr = intChkDataType (rsComm, ptrInpColl->collName, t1, rei);	/* test blbost sobota */
  /*  (void) intChkDataType (rsComm, strFullDataPath, t1, rei); */

  sprintf (strOut,
	   "OK msiChkDataType(), iCountUserDefinedMetadata=%d, t1=(%ld), iErr=%d\n",
	   iCountUserDefinedMetadata, t1, iErr);
  i = fillStrInMsParam (outParam1, strOut);	/* MsParam.c parse  addformatedtrsing to bytes WriteBytesBuff printMsParam.c */
  /* fillBuffInParam */

  printf
    ("GJK-P P.2222.0.9. in msiChkDataType(), iCountUserDefinedMetadata=%d, iErr=%d\n",
     iCountUserDefinedMetadata, iErr);

  return (iErr);
}

int
intChkDataType  (rsComm_t * rsComm, char *strFullDataPath, time_t tTime, ruleExecInfo_t * rei)
{ 
  int iCountUserDefinedMetadata = 0;
  genQueryInp_t genQueryInp;
  genQueryOut_t *genQueryOut;
  sqlResult_t *chksumStr, *modTimVal, *creaTimVal, *sqltColDataSize;
  char *objPath;
  int iIterSqlQuery = 0;
  char *tmpChksumStr, *strModTime, *strCreaTime;
  char collQCond[MAX_NAME_LEN];

  char *strColDataSize ;

  printf
    ("GJK-P P.994.12.1. in intChkDataType(), strFullDataPath=(%s)\n",
     strFullDataPath);

  /* Get all collections (recursively) under our input collection */
  /* Prepare query */
  memset (&genQueryInp, 0, sizeof (genQueryInp_t));
  memset (&genQueryOut, 0, sizeof (genQueryOut_t));
  genAllInCollQCond (strFullDataPath, collQCond);

  addInxIval (&genQueryInp.selectInp, COL_D_DATA_CHECKSUM, 1);
  addInxIval (&genQueryInp.selectInp, COL_D_MODIFY_TIME, 1);
  addInxIval (&genQueryInp.selectInp, COL_D_CREATE_TIME, 1);

  /* addInxIval (&genQueryInp.selectInp, COL_D_CREATE_TIME, 1);  #gjk1 DATA_TYPE_KW */
  addInxIval (&genQueryInp.selectInp, COL_COLL_TYPE,  1);
  addInxIval (&genQueryInp.selectInp, COL_DATA_SIZE, 1); /* #gjk2 file size in range */
  /*
    addInxIval (&genQueryInp->selectInp, COL_COLL_TYPE, 1);
    #gjk3 ACL user-uthorization pairs
    #gjk4 ACL contain at least
    #gjk5 does not ACL
    #gjk6 AVU contain exactly
    #gjk7 AVU only one
    #gjk8 AVU does not have duplicates
  */
  genQueryInp.maxRows = MAX_SQL_ROWS;

  /* ./modules/ERA/microservices/src/eraUtil.c */

  /* ICAT query for subcollections */
  rei->status = rsGenQuery (rsComm, &genQueryInp, &genQueryOut);

  printf
    ("GJK-P P.994.0.2. in intChkDataType(), rei->status=(%d), strFullDataPath=(%s), iCountUserDefinedMetadata=(%d), genQueryOut->rowCnt=(%d)\n",
     rei->status, strFullDataPath, iCountUserDefinedMetadata, genQueryOut->rowCnt);

  /* copy all subcollections */

  if (rei->status != CAT_NO_ROWS_FOUND)
    { int i; 
    dataObjInp_t destDataObjInp;     /* for rsDataObjCopy() */ 
    /* char *subCollName, *fileName;   for rsCollCreate() and rsDataObjCopy() */
    /* char *destColl, *srcColl;        full path of source and destination collections as entered */


      printf
	("GJK-P P.994.3.3. in intChkDataType(), rei->status=(%d), genQueryOut->rowCnt=(%d), strFullDataPath=(%s)\n",
	 rei->status, genQueryOut->rowCnt, strFullDataPath);

        /* copy all data objects */
      for (i=0; i < genQueryOut->rowCnt; i++) {
	printf ("GJK-P P.009.1.2. in intChkDataType(), i=(%d), destDataObjInp.objPath=(%s)\n", i, destDataObjInp.objPath);
	
      }
      
      if (1 != genQueryOut->rowCnt)
	{
	  printf
	    ("GJK-P P.994.4.4. in intChkDataType(), rei->status=(%d), genQueryOut->rowCnt=(%d), strFullDataPath=(%s)\n",
	     rei->status, genQueryOut->rowCnt, strFullDataPath);
	  /* return(-1); not enough lines); */
	}

      tmpChksumStr = NULL;

      if ((chksumStr = getSqlResultByInx (genQueryOut, COL_D_DATA_CHECKSUM)) == NULL)
	{
	  rodsLog (LOG_ERROR,
		   "printLsLong: getSqlResultByInx for COL_D_DATA_CHECKSUM failed GJK-(%s)",
		   objPath);
	  /* return (UNMATCHED_KEY_OR_INDEX); */
	}
      else
	{
	  tmpChksumStr = &chksumStr->value[chksumStr->len * iIterSqlQuery];
	  printf("GJK-P P.994.5.5. in intChkDataType(),  msiGJKExportRecursiveCollMeta(), objPath=(%s), tmpChksumStr=(%s)\n",	     objPath, tmpChksumStr);
	}

	tmpChksumStr = NULL;
	strModTime = NULL;

      if ((modTimVal = getSqlResultByInx (genQueryOut, COL_D_MODIFY_TIME)) == NULL)
	{
	  rodsLog (LOG_ERROR,		   "printLsLong: getSqlResultByInx for COL_D_MODIFY_TIME failed GJK-(%s)",		   objPath);
	  /* return (UNMATCHED_KEY_OR_INDEX); */
	}
      else
	{
	  strModTime = &modTimVal->value[modTimVal->len * iIterSqlQuery];
	  printf	    ("GJK-P P.994.5.5. in intChkDataType(),  msiGJKExportRecursiveCollMeta(), objPath=(%s), strModTime=(%s)\n",
	     objPath, strModTime);
	}

        tmpChksumStr = NULL;
strCreaTime = NULL;

      if ((creaTimVal = getSqlResultByInx (genQueryOut, COL_D_CREATE_TIME)) == NULL)
	{
	  rodsLog (LOG_ERROR,
		   "printLsLong: getSqlResultByInx for COL_D_CREATE_TIME failed GJK-(%s)",
		   objPath);
	  /* return (UNMATCHED_KEY_OR_INDEX); */
	}
      else
	{
	  strCreaTime = &creaTimVal->value[creaTimVal->len * iIterSqlQuery];
	  printf
	    ("GJK-P P.994.5.5. in intChkDataType(),  msiGJKExportRecursiveCollMeta(), objPath=(%s), strCreaTime=(%s)\n",
	     objPath, strCreaTime);
	}

tmpChksumStr = NULL;

      if ((chksumStr =	   getSqlResultByInx (genQueryOut, COL_D_DATA_CHECKSUM)) == NULL)
	{
	  rodsLog (LOG_ERROR,
		   "printLsLong: getSqlResultByInx for COL_D_DATA_CHECKSUM failed GJK-(%s)",
		   objPath);
	  /* return (UNMATCHED_KEY_OR_INDEX); */
	}
      else
	{
	  tmpChksumStr = &chksumStr->value[chksumStr->len * iIterSqlQuery];
	  printf	    ("GJK-P P.994.5.5. in intChkDataType(),  msiGJKExportRecursiveCollMeta(), objPath=(%s), COL_D_DATA_CHECKSUM=tmpChksumStr=(%s)\n",	     objPath, tmpChksumStr);
	}

strColDataSize = NULL;
tmpChksumStr = NULL;

      sqltColDataSize = getSqlResultByInx (genQueryOut, COL_DATA_SIZE);
      if (sqltColDataSize == NULL )
        {
          rodsLog (LOG_ERROR,
                   "printLsLong: getSqlResultByInx for COL_DATA_SIZE failed GJK-(%s)",
                   objPath);
          /* return (UNMATCHED_KEY_OR_INDEX); */
        }
      else
        {
	  strColDataSize = &sqltColDataSize->value[sqltColDataSize->len * iIterSqlQuery];
          printf("GJK-P P.007.5.5. in intChkDataType(),  getSqlResultByInx for COL_DATA_SIZE OK, msiGJKExportRecursiveCollMeta(), objPath=(%s), strColDataSize=(%s)\n", objPath, strColDataSize);
        }

      printf	("GJK-P P.994.6.6. (tady je moje maso AAA ZZZ) in intChkDataType(), rei->status=(%d), genQueryOut->rowCnt=(%d), strFullDataPath=(%s), strColDataSize=(%s), tmpChksumStr=(%s), iIterSqlQuery=(%d)\n",
		 rei->status, genQueryOut->rowCnt, strFullDataPath, strColDataSize, tmpChksumStr, iIterSqlQuery);
      /* tady je moje maso AAA ZZZ */



      printf	("GJK-P P.994.0.6. in intChkDataType(), after GJKgetDataObjPSmeta((%s), rsComm\n",
	 objPath);
    } /* if (rei->status != CAT_NO_ROWS_FOUND) */
  printf ("GJK-P P.994.0.8. in intChkDataType()\n");
  return(0);
}

int intAddChkSumDateAvuMetadataVol2 (rsComm_t * rsComm, char *objPath, time_t t1,			     int *iStatus)
{
  modAVUMetadataInp_t modAVUMetadataInp;
  char mytime[256], *chrPtr1;

  chrPtr1 = strrchr (objPath, '/');
  printf
    ("GJK-P P.1.0.1. in intGetDataObjChksumsTimeStampsFromAVUVol2(), chrPtr1=(%s), objPath=(%s)\n",
     chrPtr1, objPath);
  if (chrPtr1 != NULL && *chrPtr1 == '/'
      && chrPtr1[strlen (chrPtr1) - 1] == '/')
    *chrPtr1 = 0;		/* replace '/' in /myzone/foo/' */
  printf
    ("GJK-P P.1.0.2. in intGetDataObjChksumsTimeStampsFromAVUVol2(), chrPtr1=(%s), objPath=(%s)\n",
     chrPtr1, objPath);

  memset (&modAVUMetadataInp, 0, sizeof (modAVUMetadataInp));

  modAVUMetadataInp.arg0 = "add";
  modAVUMetadataInp.arg1 = "-d";	/* data */
  modAVUMetadataInp.arg2 = objPath;
  modAVUMetadataInp.arg3 = "MD5checkSumDataStamp";

  (void)
    printf
    ("GJK-P P.123.0.1. in intAddChkSumDateAvuMetadataVol2(), after rsModAVUMetadata((%s), rsComm\n",
     objPath);
#if defined(osx_platform)
  if ((long) t1 < 190509697L || (long) t1 > LONG_MAX)
#else
  if ((long) t1 < 190509697L || (long) t1 > MAXLONG)
#endif
    {
      (void) rodsLog (LOG_ERROR,
		      "The Unix time (%d) is out of reasonable bounds for intAddChkSumDateAvuMetadataVol2() for iRods data object (%s) ",
		      (int) t1, objPath);
      return (-1);
    }
  (void) snprintf (mytime, 255, "%d", (int) t1);
  modAVUMetadataInp.arg4 = mytime;
  modAVUMetadataInp.arg5 = "UnixTimeInSeconds";
  *iStatus = rsModAVUMetadata (rsComm, &modAVUMetadataInp);
  (void)
    printf
    ("GJK-P P.123.0.2. in intAddChkSumDateAvuMetadataVol2(), after rsModAVUMetadata((%s), rsComm\n",
     objPath);
  if (0 != *iStatus)
    (void) rodsLog (LOG_ERROR,
		    "intAddChkSumDateAvuMetadataVol2() rsModAVUMetadata failed objPath=(%s)",
		    objPath);
  else
    {
      (void)
	printf
	("GJK-P P.123.0.3. in intAddChkSumDateAvuMetadataVol2(), after rsModAVUMetadata((%s), sComm, mytime=%ld, *iStatus=%d\n",
	 objPath, (long)mytime, *iStatus);
    }
  (void)
    printf
    ("GJK-P P.123.0.4. OK in intAddChkSumDateAvuMetadataVol2(), after rsModAVUMetadata((%s), rsComm, *iStatus=%d\n",
     objPath, *iStatus);

  return (*iStatus);
}

int
intFindChkSumDateAvuMetadataVol2 (int status, genQueryOut_t * genQueryOut,
			    char *fullName, UserDefinedMetadata_t UAVArray[],
			    int *iCountUserDefinedMetadata)
{
  int i = 0, j = 0, iResult = 0;
  size_t size;

  *iCountUserDefinedMetadata = 0;

  printf
    ("GJK 300.0.0. intFindChkSumDateAvuMetadataVol2, fullName=(%s), i=%d, status=%d\n",
     fullName, i, status);

  /* return (0); */

  if (status != 0)
    {
      rodsLog (LOG_ERROR, "rsGenQuery");
    }
  else
    {
      if (status != CAT_NO_ROWS_FOUND)
	{
	  for (i = 0; i < (genQueryOut->rowCnt - 0); i++)
	    {
	      for (j = 0; j < (genQueryOut->attriCnt - 0); j++)
		{
		  char *tResult;

		  tResult = genQueryOut->sqlResult[j].value;
		  tResult += i * genQueryOut->sqlResult[j].len;

		  /* skip final | if no units were defined */
		  if (j < 2 || strlen (tResult))
		    {
		      size = genQueryOut->sqlResult[j].len + 2;
		    }

		  switch (j)
		    {
		    case 0:
		      strncpy (UAVArray[*iCountUserDefinedMetadata].attribute,
			       tResult, 256);
		      break;
		    case 1:
		      strncpy (UAVArray[*iCountUserDefinedMetadata].value,
			       tResult, 256);
		      break;
		    case 2:
		      strncpy (UAVArray[*iCountUserDefinedMetadata].units,
			       tResult, 256);
		      break;
		    default:
		      break;
		    }

		}		/* j=0 */
	      *iCountUserDefinedMetadata = *iCountUserDefinedMetadata + 1;
	    }			/* i=0 */
	}
    }
  iResult = *iCountUserDefinedMetadata;
  printf
    ("GJK 333.3.3. intFindChkSumDateAvuMetadataVol2, i=%d, j=%d, iCountUserDefinedMetadata=%d, iResult=%d\n",
     i, j, *iCountUserDefinedMetadata, iResult);
  return (iResult);
}

int
msiAddDataObjChksumsTimeStampsToAVUVol2 (msParam_t * inpParam1,
				     msParam_t * outParam1,
				     ruleExecInfo_t * rei)
{
  rsComm_t *rsComm;
  time_t t1;
  collInp_t collInpCache, *ptrInpColl;
  int iErr = 0, i = 0, iStatus;
  int iCountUserDefinedMetadata = 0;
  char strOut[MAX_NAME_LEN * MAX_NAME_LEN];

  /* For testing mode when used with irule --test */
  RE_TEST_MACRO
    ("RE_TEST_MACRO, begin of msiAddDataObjChksumsTimeStampsToAVUVol2");

  printf
    ("GJK-P P.1.0.0. in msiAddDataObjChksumsTimeStampsToAVUVol2(), GJK msiAddDataObjChksumsTimeStampsToAVUVol2: GJK Calling msiGetDataObjChksumsTimeStampsFromAVUVol2\n");

  rsComm = rei->rsComm;

  /*  (void) intChkDataType(rsComm, "/tempZone/home/rods/loopTest/submit.pl", (time_t) i, rei);       test blbost sobota */

  printf
    ("GJK-P P.991.0.0. in msiAddDataObjChksumsTimeStampsToAVUVol2(), GJK msiAddDataObjChksumsTimeStampsToAVUVol2: GJK Calling msiGetDataObjChksumsTimeStampsFromAVUVol2\n");

  /* parse inpParam11 */
  rei->status = parseMspForCollInp (inpParam1, &collInpCache, &ptrInpColl, 0);

  if (rei->status < 0)
    {
      rodsLog (LOG_ERROR,
	       "msiGetDataObjChksumsTimeStampsFromAVUVol2(),  input inpParam1 error. status = %d",
	       rei->status);
      return (rei->status);
    }

  (void) time (&t1);
  iErr =
    intAddChkSumDateAvuMetadataVol2 (rei->rsComm, ptrInpColl->collName, t1,
				 &iStatus);
  (void) snprintf (strOut, 255,
		   "|MD5checkSumDataStamp|%d|UnixTimeInSeconds|\n", (int) t1);
  i = fillStrInMsParam (outParam1, strOut);	/* MsParam.c parse  addformatedtrsing to bytes WriteBytesBuff printMsParam.c */

  printf
    ("GJK-P P.111.0.7. in msiGetDataObjChksumsTimeStampsFromAVUVol2(), GJK msiGetDataObjChksumsTimeStampsFromAVUVol2: GJK Calling msiGetDataObjChksumsTimeStampsFromAVUVol2, iErr=%d, iCountUserDefinedMetadata=%d\n",
     iErr, iCountUserDefinedMetadata);

  return (iErr);
}

/*
 * Get all Dates of Performed Checksum Operations from metadata AVUs for a given iRods data object.
 * 
 */

/* int msiGetDataObjChksumsTimeStampsFromAVU (msParam_t * inpParam1, msParam_t * outParam1, ruleExecInfo_t * rei) */
int msiGJK2 (msParam_t * inpParam1, msParam_t * outParam1, ruleExecInfo_t * rei)
{
  rsComm_t *rsComm;
  collInp_t collInpCache, *ptrInpColl;
  int iErr = 0, iI = 0, i = 0;
  UserDefinedMetadata_t aAVUarray[1024];
  int iCountUserDefinedMetadata = 0;
  char strOut[MAX_NAME_LEN * MAX_NAME_LEN], strTmp[1024];

  /* For testing mode when used with irule --test */
  RE_TEST_MACRO
    ("RE_TEST_MACRO, begin of msiGetDataObjChksumsTimeStampsFromAVUVol2");

  printf
    ("GJK-P P.1.0.0. in msiGetDataObjChksumsTimeStampsFromAVUVol2(), GJK msiGetDataObjChksumsTimeStampsFromAVUVol2: GJK Calling msiGetDataObjChksumsTimeStampsFromAVUVol2\n");

  rsComm = rei->rsComm;

  /* parse inpParam11 */
  rei->status = parseMspForCollInp (inpParam1, &collInpCache, &ptrInpColl, 0);

  if (rei->status < 0)
    {
      rodsLog (LOG_ERROR,
	       "msiGetDataObjChksumsTimeStampsFromAVUVol2(),  input inpParam1 error. status = %d",
	       rei->status);
      return (rei->status);
    }

  iErr =
    intGetDataObjChksumsTimeStampsFromAVUVol2 (ptrInpColl, aAVUarray,
					   &iCountUserDefinedMetadata, strOut,
					   rei);

  printf
    ("GJK-P P.111.0.7. in msiGetDataObjChksumsTimeStampsFromAVUVol2(), GJK msiGetDataObjChksumsTimeStampsFromAVUVol2: GJK Calling msiGetDataObjChksumsTimeStampsFromAVUVol2, iErr=%d, iCountUserDefinedMetadata=%d\n",
     iErr, iCountUserDefinedMetadata);

  for (iI = 0; iI < iCountUserDefinedMetadata; iI++)
    {

      snprintf (strTmp, MAX_NAME_LEN, "|%s|%s|%s|\n",
		aAVUarray[iI].attribute, aAVUarray[iI].value,
		aAVUarray[iI].units);
      strncat (strOut, strTmp, MAX_NAME_LEN);
    }

  /*    sprintf(strOut, "#1\n#2\n\n#3 lines gjk\n"); */
  i = fillStrInMsParam (outParam1, strOut);	/* MsParam.c parse  addformatedtrsing to bytes WriteBytesBuff printMsParam.c */
  /* fillBuffInParam */


  printf
    ("GJK-P P.111.0.9. in msiGetDataObjChksumsTimeStampsFromAVUVol2(), GJK msiGetDataObjChksumsTimeStampsFromAVUVol2: GJK Calling msiGetDataObjChksumsTimeStampsFromAVUVol2, iCountUserDefinedMetadata=%d\n",
     iCountUserDefinedMetadata);

  return (iErr);
}

/* ****************************************************************************************** */
int
intGetDataObjChksumsTimeStampsFromAVUVol2 (collInp_t * ptrInpColl,
				       UserDefinedMetadata_t * aAVUarray,
				       int *iTotalAVUs, char *strOut,
				       ruleExecInfo_t * rei)
{
  char *chrPtr1 =
    NULL, strAbsPath[MAX_NAME_LEN], v1[1024], v2[1024], v3[1024],
    strDirName[MAX_NAME_LEN], strFileName[MAX_NAME_LEN], *condVal[10],
    attrName[256] = "MD5checkSumDataStamp";;
  genQueryInp_t genQueryInp;
  int i1a[10], i1b[10], i2a[10], iI = 0, iErr = 0, printCount = 0;
  genQueryOut_t *genQueryOut;

  chrPtr1 = strrchr (ptrInpColl->collName, '/');
  printf
    ("GJK-P P.21.0.1. in intGetDataObjChksumsTimeStampsFromAVUVol2(), chrPtr1=(%s), ptrInpColl->collName=(%s)\n",
     chrPtr1, ptrInpColl->collName);
  if (chrPtr1 != NULL && *chrPtr1 == '/'
      && (ptrInpColl->collName[strlen (ptrInpColl->collName) - 1] == '/'))
    *chrPtr1 = 0;		/* replace '/' in /myzone/foo/' */
/*
  else printf
  ("GJK-P P.21.1.1. in intGetDataObjChksumsTimeStampsFromAVUVol2(), chrPtr1=(%s), ptrInpColl->collName=(%s), Pmath=%d, strlen=%d\n",
  chrPtr1, ptrInpColl->collName, (int)(chrPtr1 - ptrInpColl->collName), (strlen (ptrInpColl->collName) - 1));
  */

  /* spatne !!!if (chrPtr1 != NULL && *chrPtr1 == '/' &&)    *chrPtr1 = 0;           replace '/' in /myzone/foo/' */
  printf
    ("GJK-P P.21.0.2. in intGetDataObjChksumsTimeStampsFromAVUVol2(), chrPtr1=(%s), ptrInpColl->collName=(%s)\n",
     chrPtr1, ptrInpColl->collName);

  if ((iI = isData (rei->rsComm, ptrInpColl->collName, NULL)) >= 0)
    {
      rodsLog (LOG_NOTICE,
	       "GJK intGetDataObjChksumsTimeStampsFromAVUVol2: input (%s) is data.",
	       ptrInpColl->collName);
    }
  else
    {
      printf
	("GJK-P P.21.0.3. in intGetDataObjChksumsTimeStampsFromAVUVol2(), chrPtr1=(%s), ptrInpColl->collName=(%s), iI=%d\n",
	 chrPtr1, ptrInpColl->collName, iI);
      if ((iI = isColl (rei->rsComm, ptrInpColl->collName, NULL)) < 0)
	{
	  rodsLog (LOG_ERROR,
		   "iGetDataObjChksumsTimeStampsFromAVU: input object=(%s) is not data or collection. Exiting!",
		   ptrInpColl->collName);
	  /* Ereturn (rei->status); */
	}
      else
	{
	  rodsLog (LOG_ERROR,
		   "GJK intGetDataObjChksumsTimeStampsFromAVUVol2: input (%s) is a collection.",
		   ptrInpColl->collName);
	  return (rei->status);
	}
    }


  if (rei->rsComm == NULL)
    {
      rodsLog (LOG_ERROR, "GJKgetDataObjPSmeta: input rsComm is NULL");
      return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

  memset (&genQueryInp, 0, sizeof (genQueryInp_t));

  i1a[0] = COL_META_DATA_ATTR_NAME;
  i1b[0] = 0;			/* currently unused */
  i1a[1] = COL_META_DATA_ATTR_VALUE;
  i1b[1] = 0;			/* currently unused */
  i1a[2] = COL_META_DATA_ATTR_UNITS;
  i1b[2] = 0;			/* currently unused */
  genQueryInp.selectInp.inx = i1a;
  genQueryInp.selectInp.value = i1b;
  genQueryInp.selectInp.len = 3;

  strncpy (strAbsPath, ptrInpColl->collName, MAX_NAME_LEN);
  printf
    ("GJK-P P.14.0.11. in intGetDataObjChksumsTimeStampsFromAVUVol2(), strAbsPath=(%s), ptrInpColl->collName=(%s)\n",
     strAbsPath, ptrInpColl->collName);

  iErr = splitPathByKey (strAbsPath, strDirName, strFileName, '/');

  printf
    ("GJK-P P.14.0.12. in intGetDataObjChksumsTimeStampsFromAVUVol2(), strAbsPath=(%s), ptrInpColl->collName=(%s), strDirName=(%s), strFileName=(%s), iErr=%d\n",
     strAbsPath, ptrInpColl->collName, strDirName, strFileName, iErr);

  i2a[0] = COL_COLL_NAME;
  sprintf (v1, "='%s'", strDirName);
  condVal[0] = v1;

  i2a[1] = COL_DATA_NAME;
  sprintf (v2, "='%s'", strFileName);
  condVal[1] = v2;

  i2a[3] = COL_DATA_SIZE;
  /* sprintf (v4, "='%s'", strFileSize); */
  condVal[3] = v3;


  genQueryInp.sqlCondInp.inx = i2a;
  genQueryInp.sqlCondInp.value = condVal;
  genQueryInp.sqlCondInp.len = 3;

  if (attrName != NULL && *attrName != '\0')
    {
      i2a[2] = COL_META_DATA_ATTR_NAME;
      sprintf (v3, "= '%s'", attrName);
      condVal[2] = v3;
      genQueryInp.sqlCondInp.len++;
    }

  genQueryInp.maxRows = 100;
  genQueryInp.continueInx = 0;
  genQueryInp.condInput.len = 0;

  printf
    ("GJK-P P.14.0.13. in intGetDataObjChksumsTimeStampsFromAVUVol2(), strAbsPath=(%s), ptrInpColl->collName=(%s), v3=(%s), iErr=%d\n",
     strAbsPath, ptrInpColl->collName, v3, iErr);

  /* Actual query happens here */
  iErr = rsGenQuery (rei->rsComm, &genQueryInp, &genQueryOut);

  printf
    ("GJK-P P.14.0.14. in intGetDataObjChksumsTimeStampsFromAVUVol2(), strAbsPath=(%s), ptrInpColl->collName=(%s), iErr=%d\n",
     strAbsPath, ptrInpColl->collName, iErr);

  if (iErr == CAT_NO_ROWS_FOUND)
    {
      i1a[0] = COL_D_DATA_PATH;
      genQueryInp.selectInp.len = 1;
      iErr = rsGenQuery (rei->rsComm, &genQueryInp, &genQueryOut);
      if (iErr == 0)
	{
	  printf ("GJK GJKgetDataObjPSmeta(),  iErr=%d, None\n", iErr);
	  return (0);
	}
      if (iErr == CAT_NO_ROWS_FOUND)
	{

	  rodsLog (LOG_NOTICE,
		   "GJKgetDataObjPSmeta: DataObject %s not found. iErr = %d",
		   strAbsPath, iErr);
	  *iTotalAVUs = 0;
	  return (0);
	}
      printCount += intFindChkSumDateAvuMetadataVol2 (iErr, genQueryOut, strAbsPath, aAVUarray, iTotalAVUs);	/* why?? */
    }
  else
    {
      printCount += intFindChkSumDateAvuMetadataVol2 (iErr, genQueryOut, strAbsPath, aAVUarray, iTotalAVUs);	/* why?? */ 
    }

  while (iErr == 0 && genQueryOut->continueInx > 0)
    {
      genQueryInp.continueInx = genQueryOut->continueInx;
      iErr = rsGenQuery (rei->rsComm, &genQueryInp, &genQueryOut);
      printCount += intFindChkSumDateAvuMetadataVol2 (iErr, genQueryOut, strAbsPath, aAVUarray, iTotalAVUs);	/* why ?? */
    }

  printf
    ("GJK-P P.14.1.15. in intGetDataObjChksumsTimeStampsFromAVUVol2(), strAbsPath=(%s), ptrInpColl->collName=(%s), iErr=%d, *iTotalAVUs=%d\n",
     strAbsPath, ptrInpColl->collName, iErr, *iTotalAVUs);

  return (*iTotalAVUs);
}
