#include "transaction_manager_struct.h"
#include "globalvar.h"
#include "site.h"
#include <ctype.h>
#include <sys/select.h>



#define WAIT_SITE_FAIL

#define SLEEP_DURATION_MS 	200

int availableSites[MAXIMUM_SITES] ;
int storeOperation(char *operationString, int operationtimestamp) ;
int prepareOperationNode(int trnid, int opnType, int variablenumber, int writtenValue, int sitenumber, int operationtimestamp, struct operation *opn) ;
void addOperationToTransactionQueue(int trnid, struct operation *opn) ;
void getSites(int variablenumber, int trySites[]) ;
int createNewTransaction(int trnid, int trnType, int timestamp) ;
void abortTransaction(struct operation *opn) ;
void Sleep_ms(int time_ms) ;

#define WRITE_PENDING  1
#define WRITE_CHECK_FOR_COMPLETE 0
#define WRITE_FAILED   -1

void startTransactionManager() 
{
  int tickNo = 0 ;
  int trnid ;
  int flagPending = 1 ;
  char log_desc[1000] ;
  while(flagPending != 0) 
  {
    sprintf(log_desc, "\n\n\n _____________________________________START TRANSACTION MANAGER  : Tick# %d_________________________________\n\n", tickNo) ;
    logString(log_desc) ;
    flagPending = 0 ;


    while( T[OTHER_TRNID].current_operation != NULL ) 
	{
      if(flagPending == 0)
	flagPending = 1 ;
      if(T[OTHER_TRNID].current_operation->opnTimestamp > tickNo) {
        break ;
      }
      if(T[OTHER_TRNID].timestamp <= tickNo && T[OTHER_TRNID].current_operation->opnTimestamp <= tickNo) {
        struct operation *opn = T[OTHER_TRNID].current_operation ;
        if(opn->opnType == DUMP_OPN)
	{	
          if(opn->variablenumber == ALL_VARIABLES) {
            if(opn->sitenumber == ALL_SITES) {	
              int sitenumber ;
              for(sitenumber = 1 ; sitenumber < MAXIMUM_SITES ; sitenumber++) {
                if(siteInfo[sitenumber].flag_site_available == 0)	
                  continue ;
                performOperation(opn, sitenumber) ;		
                int operationStatus = opn->opnSiteStatus[sitenumber] ;
#ifdef _DEBUG_
                if(operationStatus == OPN_REJECTED)
                 printf("DUMP is REJECTED at site : %d\n", sitenumber) ;
                if(operationStatus == OPN_BLOCKED)
                 printf("DUMP is BLOCKED at site : %d\n", sitenumber) ;
#endif
                if(operationStatus != OPN_COMPLETE) {
#ifdef _DEBUG_
                  printf("START TRANSACTION MANAGER: Site : %d did not set DUMP as complete. \n", sitenumber ) ;
#endif
                  opn->opnSiteStatus[sitenumber] = OPN_COMPLETE ;
                }
              }
            }
            else if(opn->sitenumber != ALL_SITES) {		
              if(siteInfo[opn->sitenumber].flag_site_available == 0) {	
               printf("START TRANSACTION MANAGER: Received DUMP instruction for failed site : %d\n", opn->sitenumber ) ;
              }
              else {
                performOperation(opn, opn->sitenumber) ;
                int operationStatus = opn->opnSiteStatus[opn->sitenumber] ;
#ifdef _DEBUG_
                if(operationStatus == OPN_REJECTED)
                 printf("DUMP is REJECTED at site : %d\n", opn->sitenumber) ;
                if(operationStatus == OPN_BLOCKED)
                 printf("DUMP is BLOCKED at site : %d\n", opn->sitenumber) ;
#endif
                if(operationStatus != OPN_COMPLETE) {
#ifdef _DEBUG_
                  printf("startTransactionManager: site %d did not set dump operation state to complete\n", opn->sitenumber ) ;
#endif
                  opn->opnSiteStatus[opn->sitenumber] = OPN_COMPLETE ;
                  int operationStatus = opn->opnSiteStatus[opn->sitenumber] ;
#ifdef _DEBUG_
                  if(operationStatus == OPN_REJECTED)
                   printf("Dump operation rejected @ site %d\n", opn->sitenumber) ;
                  if(operationStatus == OPN_BLOCKED)
                   printf("Dump operation @ site %d\n", opn->sitenumber) ;
#endif
                  if(operationStatus != OPN_COMPLETE) {
#ifdef _DEBUG_
                    printf("START TRANSACTION MANAGER: Site : %d did not set DUMP as complete. \n", opn->sitenumber ) ;
#endif
                    opn->opnSiteStatus[opn->sitenumber] = OPN_COMPLETE ;
                  }
                }
              }
            }
          }
          else if(opn->variablenumber != ALL_VARIABLES) {	
            int sitenumber ;
            if(opn->variablenumber % 2 == 1) {	
              sitenumber = (opn->variablenumber % 10) + 1 ;
              if(siteInfo[sitenumber].flag_site_available == 0 ) {
                printf("START TRANSACTION MANAGER: DUMP excecuted on X %d cannot be performed at SITE %d since the site is in failed status.\n", opn->variablenumber, sitenumber ) ;
              }
              else {
               performOperation(opn, sitenumber) ;
               int operationStatus = opn->opnSiteStatus[sitenumber] ;
#ifdef _DEBUG_
               if(operationStatus == OPN_REJECTED)
                printf("DUMP is REJECTED at site : %d\n", sitenumber) ;
               if(operationStatus == OPN_BLOCKED)
                printf("DUMP is EXECUTED at site : %d\n", sitenumber) ;
#endif
               if(operationStatus != OPN_COMPLETE) {
#ifdef _DEBUG_
                 printf("START TRANSACTION MANAGER: Site : %d did not set DUMP as complete. \n", sitenumber ) ;
#endif
                 opn->opnSiteStatus[sitenumber] = OPN_COMPLETE ;
               }
              }
            }
            else {	
              int sitenumber ;
              for(sitenumber = 1 ; sitenumber < MAXIMUM_SITES ; sitenumber++) {
                if(siteInfo[sitenumber].flag_site_available == 0)	
                  continue ;
                performOperation(opn, sitenumber) ;		
                int operationStatus = opn->opnSiteStatus[sitenumber] ;
#ifdef _DEBUG_
                if(operationStatus == OPN_REJECTED)
                 printf("DUMP is REJECTED at site : %d\n", sitenumber) ;
                if(operationStatus == OPN_BLOCKED)
                 printf("DUMP is BLOCKED at site : %d\n", sitenumber) ;
#endif
                if(operationStatus != OPN_COMPLETE) {
#ifdef _DEBUG_
                  printf("START TRANSACTION MANAGER: Site : %d did not set DUMP as complete. \n", sitenumber ) ;
#endif
                  opn->opnSiteStatus[sitenumber] = OPN_COMPLETE ;
                }
              }
            }
          }
        }
        else if(opn->opnType == QUERY_STATE_OPN ) {
	  int sitenumber ;
	  for(sitenumber = 1 ; sitenumber < MAXIMUM_SITES ; sitenumber++) {
	    if(siteInfo[sitenumber].flag_site_available == 0)    
	      continue ;
	    performOperation(opn, sitenumber) ;         
	    int operationStatus = opn->opnSiteStatus[sitenumber] ;
#ifdef _DEBUG_
	    if(operationStatus == ON_REJECTED)
	    {
	        sprintf(log_desc,"QUERY STATE is REJECTED at site : %d\n", sitenumber) ;
		logString(log_desc) ;
	    }
	    if(operationStatus == ON_BLOCKED)
	    {   sprintf(log_desc,"QUERY STATE is BLOCKED at site : %d\n %d\n", sitenumber) ;
		logString(log_desc) ;
	    }
#endif
	    if(operationStatus != OPN_COMPLETE) {
#ifdef _DEBUG_
	      sprintf(log_desc,"START TRANSACTION MANAGER: Site : %d did not set QUERY STATE as complete. \n", sitenumber ) ;
	      logString(log_desc) ;
#endif
	      opn->opnSiteStatus[sitenumber] = OPN_COMPLETE ;
	    }
          }
	  int transactionID ;
	  for(transactionID = 0; transactionID < MAXIMUM_TRANSACTIONS ; transactionID++ ) {
	    if(transactionID == OTHER_TRNID || T[transactionID].timestamp > tickNo || T[transactionID].timestamp == -1) {
	      continue ;
	    }
	    if(T[transactionID].current_operation == NULL) {	
	      if(T[transactionID].trn_Completion_Status == TRN_COMMITED) {
		sprintf(log_desc, "START TRANSACTION MANAGER: QUERY STATE for TRANSACTION %d is COMMITED\n", transactionID) ;
                logString(log_desc) ;
	      }
	      else {
		sprintf(log_desc, "START TRANSACTION MANAGER: QUERY STATE for TRANSACTION %d is ABORTED\n", transactionID) ;
                logString(log_desc) ;
	      }
	    }
	    else if(T[transactionID].current_operation->opnTimestamp < tickNo){
	      if(T[transactionID].current_operation->opnType == READ_OPN) {
		sprintf(log_desc, "START TRANSACTION MANAGER: QUERY STATE for TRANSACTION %d is WAITING for READ on X %d arrived at Tick# %d to be completed\n", transactionID, T[transactionID].current_operation->variablenumber, T[transactionID].current_operation->opnTimestamp) ;
                logString(log_desc) ;
	      }
	      else if(T[transactionID].current_operation->opnType == WRITE_OPN) { 
		sprintf(log_desc, "START TRANSACTION MANAGER: QUERY STATE for TRANSACTION %d is WAITING for WRITE on X%d with value %d arrived at Tick# %d to be completed\n", transactionID, T[transactionID].current_operation->variablenumber, T[transactionID].current_operation->writtenValue, T[transactionID].current_operation->opnTimestamp) ;
                logString(log_desc) ;
	      }
	    }
	    else if(T[transactionID].current_operation->opnTimestamp >= tickNo) {
		sprintf(log_desc, "START TRANSACTION MANAGER: QUERY STATE for TRANSACTION %d is WAITING for NEW operation to arrive from input Sequence\n", transactionID, T[transactionID].current_operation->variablenumber, T[transactionID].current_operation->writtenValue, T[transactionID].current_operation->opnTimestamp) ;
                logString(log_desc) ;

	    }
	  }
        }
        else if(opn->opnType == FAIL_OPN || opn->opnType == RECOVER_OPN) {
          performOperation(opn, opn->sitenumber) ;
          if(opn->opnType == FAIL_OPN ) {
	   sprintf(log_desc,"START TRANSACTION MANAGER: Site %d is now FAILED\n", opn->sitenumber ) ;
	  
           siteInfo[opn->sitenumber].flag_site_available = 0 ;
	  } 
          else {
	   if(siteInfo[opn->sitenumber].flag_site_available == 0) { 
             siteInfo[opn->sitenumber].flag_site_available = 1 ;
	     sprintf(log_desc,"START TRANSACTION MANAGER: Site %d is now RECOVERED\n", opn->sitenumber ) ;
	     
             siteInfo[opn->sitenumber].tick_time = tickNo ;		
	   }
           else {
             sprintf(log_desc,"START TRANSACTION MANAGER: Site %d has been RECOVERED no failure on site earlier.\n", opn->sitenumber ) ;
	     
           }
          }
        }
        T[OTHER_TRNID].current_operation = T[OTHER_TRNID].current_operation->opnTM ;
      } 
    }
    for(trnid = 0 ; trnid < MAXIMUM_TRANSACTIONS ; trnid++) {
      if(trnid  == OTHER_TRNID)
        continue ;
      if(T[trnid].current_operation != NULL ) {
	if(flagPending == 0) {
	  flagPending = 1 ;
	} 
	if(T[trnid].timestamp > tickNo) {		
	  continue ; 
	}
        if(T[trnid].current_operation->opnTimestamp <= tickNo) {		
          struct operation *opn = T[trnid].current_operation ;
          if(opn->opnType == READ_OPN ) {
            if(opn->variablenumber % 2 == 1) {	
              int sitenumber = (opn->variablenumber % 10) + 1 ;
              opn->sitenumber = sitenumber ;
              if(opn->opnSiteStatus[sitenumber] == OPN_PENDING )
	      {	
                if(siteInfo[sitenumber].flag_site_available == 0 ) {		
#ifdef ABORT_SITE_FAIL
                  sprintf(log_desc,"START TRANSACTION MANAGER: TRANSACTION: %d ABORTED since READ on var %d failed due to site %d failure\n", trnid, opn->variablenumber, opn->sitenumber) ;
		  logString(log_desc);
                  T[trnid].current_operation = NULL ;			
#endif
#ifdef WAIT_SITE_FAIL
                  if(opn->waitlistedOpnTicknumber == -1 ) {
                    opn->waitlistedOpnTicknumber = tickNo ;
                  }
                  opn->opnSiteStatus[sitenumber] = OPN_PENDING ;
                  sprintf(log_desc, "START TRANSACTION MANAGER: TRANSACTION: %d BLOCKED at READ on var %d since site %d has temporarily failed. Retrying on next Tick..\n", trnid, opn->variablenumber, sitenumber) ;
                  logString(log_desc) ;
                  continue ;
#endif
                }
                else {
                  opn->sitenumber = sitenumber ;
                  opn->waitlistedOpnTicknumber = -1 ;
                  performOperation(opn, sitenumber) ;					
                  if(opn->opnSiteStatus[sitenumber] == OPN_REJECTED ) {
                    sprintf(log_desc,"START TRANSACTION MANAGER: TRANSACTION: %d ABORTED since READ on var %d rejected by site %d\n", trnid, opn->variablenumber, opn->sitenumber) ;
                    logString(log_desc) ;
                    abortTransaction(opn) ;
                    T[trnid].trn_Completion_Status = TRN_ABORTED ;
                    T[trnid].current_operation = NULL ;
                  }
                  else if(opn->opnSiteStatus[sitenumber] == OPN_BLOCKED) {
                    sprintf(log_desc, "START TRANSACTION MANAGER: TRANSACTION: %d BLOCKED for READ on var %d at site %d since the site could not provide the lock\n", trnid, opn->variablenumber, opn->sitenumber) ;
                    logString(log_desc) ;
                    
                  }
                  else if(opn->opnSiteStatus[sitenumber] == OPN_COMPLETE) {
                    
                    if(T[trnid].accessedSites[sitenumber].firstAccessedTick == -1) {       
                      T[trnid].accessedSites[sitenumber].firstAccessedTick = tickNo ;
                    }
                    T[trnid].current_operation = T[trnid].current_operation->opnTM ;
                    sprintf(log_desc, "START TRANSACTION MANAGER: TRANSACTION: %d READ on var no. %d returns %d from site %d\n", trnid, opn->variablenumber, opn->readValue, opn->sitenumber) ;
                    logString(log_desc) ;
                  }
                }
              }
              else if(opn->opnSiteStatus[sitenumber] == OPN_BLOCKED) {	
                if(siteInfo[sitenumber].flag_site_available == 0 ) {
#ifdef ABORT_SITE_FAIL
                  printf("START TRANSACTION MANAGER: TRANSACTION: %d ABORTED since READ on var %d at site %d timed out\n", trnid, opn->variablenumber, opn->sitenumber ) ;
                  abortTransaction(opn) ;
                  T[trnid].trn_Completion_Status = TRN_ABORTED ;
                  T[trnid].current_operation = NULL ;
#endif
#ifdef WAIT_SITE_FAIL
		  if(opn->waitlistedOpnTicknumber == -1 ) {
		    opn->waitlistedOpnTicknumber = tickNo ;    
		  }
		  sprintf(log_desc,"START TRANSACTION MANAGER: TRANSACTION: %d has BLOCKED at READ on var %d at since site %d has temporarily failed\n", trnid, opn->variablenumber, opn->sitenumber ) ;
                  logString(log_desc) ;
		  opn->opnSiteStatus[sitenumber] = OPN_PENDING ;
		} 
#endif
              }
              else if(opn->opnSiteStatus[sitenumber] == OPN_COMPLETE) {
                if(T[trnid].accessedSites[sitenumber].firstAccessedTick == -1) {        
                  T[trnid].accessedSites[sitenumber].firstAccessedTick = tickNo ;
                }
                T[trnid].current_operation = T[trnid].current_operation->opnTM ;
                sprintf(log_desc, "START TRANSACTION MANAGER: TRANSACTION: %d READ on var no. %d returns %d from site %d\n", trnid, opn->variablenumber, opn->readValue, opn->sitenumber) ;
                logString(log_desc) ;
              }
            }
            else {	
              RETRY_READ:
              if(opn->sitenumber == -1) {
                opn->sitenumber = 1 ;	
              }
              if(siteInfo[opn->sitenumber].flag_site_available == 0) {	
                while(siteInfo[opn->sitenumber].flag_site_available == 0 && opn->sitenumber < MAXIMUM_SITES) {
                  opn->sitenumber++ ;
                }
                if(opn->sitenumber == MAXIMUM_SITES) {	
#ifdef ABORT_SITE_FAIL
                  printf("START TRANSACTION MANAGER: TRANSACTION: %d ABORTED since READ on var %d could not be completed at any of the sites\n", trnid, opn->variablenumber) ;
                  abortTransaction(opn) ;
                  T[trnid].trn_Completion_Status = TRN_ABORTED ;
                  T[trnid].current_operation = NULL ;
                  continue ;
#endif
#ifdef WAIT_SITE_FAIL
                  if(opn->waitlistedOpnTicknumber == -1) {
                    opn->waitlistedOpnTicknumber = tickNo ;
                  }
                  int i = 1 ;
                  for(i = 1; i < MAXIMUM_SITES; i++) {		//We will retry this operation again at all sites
                    opn->opnSiteStatus[i] = OPN_PENDING ;
                  }
                  opn->sitenumber = 1 ;
                  sprintf(log_desc,"START TRANSACTION MANAGER: TRANSACTION : %d BLOCKED at READ on var %d since all sites have failed. Retrying on the sites at the next Tick..\n", trnid, opn->variablenumber) ;
                  logString(log_desc) ;
                  continue ;
#endif
                }
              }
              if(opn->opnSiteStatus[opn->sitenumber] == OPN_PENDING) {
                opn->waitlistedOpnTicknumber = -1 ;
                performOperation(opn, opn->sitenumber) ;                                       
                if(opn->opnSiteStatus[opn->sitenumber] == OPN_REJECTED ) {
                  
                  opn->sitenumber++ ;
                  if(opn->sitenumber == MAXIMUM_SITES) {        
                    abortTransaction(opn) ;
                    T[trnid].trn_Completion_Status = TRN_ABORTED ;
                    sprintf(log_desc, "START TRANSACTION MANAGER: TRANSACTION: %d ABORTED since READ on variablenumber %d since the operation could not be completed at any site\n", trnid, opn->variablenumber) ;
		    logString(log_desc);	
                    T[trnid].current_operation = NULL ;
                    continue ;
                  }
                  else {
                    goto RETRY_READ ;
                  }
                }
                else if(opn->opnSiteStatus[opn->sitenumber] == OPN_BLOCKED) {
                  
                  sprintf(log_desc, "START TRANSACTION MANAGER: TRANSACTION: %d BLOCKED for READ on var %d at site %d since site could not provide the lock\n", trnid, opn->variablenumber, opn->sitenumber) ;
                  logString(log_desc) ;
                }
                else if(opn->opnSiteStatus[opn->sitenumber] == OPN_COMPLETE) {
                  
                  T[trnid].current_operation = T[trnid].current_operation->opnTM ;
                  if(T[trnid].accessedSites[opn->sitenumber].firstAccessedTick == -1) {        
                    T[trnid].accessedSites[opn->sitenumber].firstAccessedTick = tickNo ;
                  }
                  sprintf(log_desc, "START TRANSACTION MANAGER: TRANSACTION: %d READ on var no. %d returns %d from site %d\n", trnid, opn->variablenumber, opn->readValue, opn->sitenumber) ;
                  logString(log_desc) ;
                }
              }
              else if(opn->opnSiteStatus[opn->sitenumber] == OPN_BLOCKED) {      
                if(siteInfo[opn->sitenumber].flag_site_available == 0 ) {  
                  opn->sitenumber++ ;
                  if(opn->sitenumber == MAXIMUM_SITES) {	
#ifdef ABORT_SITE_FAIL
                    printf("START TRANSACTION MANAGER: TRANSACTION: %d ABORTED since READ on var %d could not be completed at any of the sites\n", trnid, opn->variablenumber) ;
                    abortTransaction(opn) ;
                    T[trnid].trn_Completion_Status = TRN_ABORTED ;
                    T[trnid].current_operation = NULL ;
                    continue ;
#endif
#ifdef WAIT_SITE_FAIL
                    if(opn->waitlistedOpnTicknumber == -1) {
                      opn->waitlistedOpnTicknumber = tickNo ;
                    }
                    int i = 1 ;
                    for(i = 1; i < MAXIMUM_SITES; i++) {		
                      opn->opnSiteStatus[i] = OPN_PENDING ;
                    }
                    opn->sitenumber = 1 ;
                    sprintf(log_desc,"START TRANSACTION MANAGER: TRANSACTION : %d BLOCKED at READ on var %d since all sites have failed. Retrying on the sites at the next Tick..\n", trnid, opn->variablenumber) ;
                    logString(log_desc) ;
                    continue ;
#endif
                  }
		  else { 
                    sprintf(log_desc, "START TRANSACTION MANAGER: TRANSACTION %d READ at site %d for variablenumber %d failed due to site failure, Retrying on next available site\n", trnid, opn->sitenumber-1, opn->variablenumber) ;
                    logString(log_desc) ;
                  }
                }
                else {
                  sprintf(log_desc, "START TRANSACTION MANAGER: TRANSACTION %d WAITING for READ at site %d for variablenumber %d to be complete\n", trnid, opn->sitenumber, opn->variablenumber) ;
                  
                  printf("%s", log_desc) ;
                }

              }
              else if(opn->opnSiteStatus[opn->sitenumber] == OPN_COMPLETE) {
                if(T[trnid].accessedSites[opn->sitenumber].firstAccessedTick == -1) {        
                  T[trnid].accessedSites[opn->sitenumber].firstAccessedTick = tickNo ;
                }
                sprintf(log_desc, "START TRANSACTION MANAGER: TRANSACTION: %d READ on var no. %d returns %d from site %d\n", trnid, opn->variablenumber, opn->readValue, opn->sitenumber) ;
                logString(log_desc) ;
                T[trnid].current_operation = T[trnid].current_operation->opnTM ;
              }
            }
          }

          else if(opn->opnType == WRITE_OPN ) {
            if(opn->variablenumber % 2 == 1) {	
              int sitenumber = (opn->variablenumber % 10) + 1 ;
              opn->sitenumber = sitenumber ;
              if(opn->opnSiteStatus[sitenumber] == OPN_PENDING ) {	
                if(siteInfo[sitenumber].flag_site_available == 0 ) {		

#ifdef ABORT_SITE_FAIL
                  printf("START TRANSACTION MANAGER: TRANSACTION: %d ABORTED since WRITE on var %d with value %d failed due to site %d failure\n", trnid, opn->variablenumber, opn->writtenValue, opn->sitenumber) ;
                  T[trnid].current_operation = NULL ;			
#endif
#ifdef WAIT_SITE_FAIL
                  if(opn->waitlistedOpnTicknumber == -1 ) {
                    opn->waitlistedOpnTicknumber = tickNo ;
                  }
                  opn->opnSiteStatus[sitenumber] = OPN_PENDING ;
                  sprintf(log_desc, "START TRANSACTION MANAGER: TRANSACTION: %d BLOCKED at WRITE on var %d with value %d since site %d has temporarily failed\n", trnid, opn->variablenumber, opn->writtenValue, opn->sitenumber) ;
                  logString(log_desc) ;
#endif
                }
                else {
                  opn->sitenumber = sitenumber ;
                  opn->waitlistedOpnTicknumber = -1 ;
                  performOperation(opn, sitenumber) ;					
                  if(opn->opnSiteStatus[sitenumber] == OPN_REJECTED ) {
                    sprintf(log_desc, "START TRANSACTION MANAGER: Transaction %d ABORTED since write on variablenumber %d with value %d rejected by site %d\n", trnid, opn->variablenumber, opn->writtenValue, sitenumber) ;
                    logString(log_desc) ;
                    abortTransaction(opn) ;
                    T[trnid].trn_Completion_Status = TRN_ABORTED ;
                    T[trnid].current_operation = NULL ;
                  }
                  else if(opn->opnSiteStatus[sitenumber] == OPN_BLOCKED) {
                    sprintf(log_desc,"START TRANSACTION MANAGER: Transaction: %d BLOCKED for write on variablenumber %d with value %d at site %d since site could not provide it with the lock\n", trnid, opn->variablenumber, opn->writtenValue, opn->sitenumber) ;
                    logString(log_desc) ;
                    
                  }
                  else if(opn->opnSiteStatus[sitenumber] == OPN_COMPLETE) {
                    
                   sprintf(log_desc, "START TRANSACTION MANAGER: Transaction: %d WRITE on var %d with value %d completed\n", trnid, opn->variablenumber, opn->writtenValue, opn->sitenumber) ;
                   logString(log_desc) ;
                    T[trnid].current_operation = T[trnid].current_operation->opnTM ;
                    if(T[trnid].accessedSites[sitenumber].firstAccessedTick == -1) {        
                      T[trnid].accessedSites[sitenumber].firstAccessedTick = tickNo ;
                    }
                    if(T[trnid].accessedSites[sitenumber].writeAccessed_Flag == 0) {
                      T[trnid].accessedSites[sitenumber].writeAccessed_Flag = 1 ;       
                    }
                  }
                }
              }
              else if(opn->opnSiteStatus[sitenumber] == OPN_BLOCKED) {	
                if(siteInfo[sitenumber].flag_site_available == 0 ) {
#ifdef ABORT_SITE_FAIL
                  printf("START TRANSACTION MANAGER: Transaction %d ABORTED since WRITE on variablenumber %d with value to be written %d at site %d timedout\n", trnid, opn->variablenumber, opn->writtenValue, opn->sitenumber ) ;
                  abortTransaction(opn) ;
                  T[trnid].trn_Completion_Status = TRN_ABORTED ;
                  T[trnid].current_operation = NULL ;
#endif
#ifdef WAIT_SITE_FAIL
		  if(opn->waitlistedOpnTicknumber == -1 ) {
		    opn->waitlistedOpnTicknumber = tickNo ;    //Record the tickNo at which the operation was blocked due to site failure
		  }
		  sprintf(log_desc, "START TRANSACTION MANAGER: TRANSACTION: %d BLOCKED at WRITE on var %d with value %d since site %d has temporarily failed\n", trnid, opn->variablenumber, opn->writtenValue, opn->sitenumber ) ;
                  logString(log_desc) ;
		  opn->opnSiteStatus[sitenumber] = OPN_PENDING ;
#endif
                }
                else {
                    sprintf(log_desc, "START TRANSACTION MANAGER: Transaction: %d still WAITING for write on var %d with value %d BLOCKED at site %d\n", trnid, opn->variablenumber, opn->writtenValue, opn->sitenumber) ;
                    //logString(log_desc) ;
                    printf("%s", log_desc) ;
                }
              }
              else if(opn->opnSiteStatus[sitenumber] == OPN_COMPLETE) {
                sprintf(log_desc, "START TRANSACTION MANAGER: Transaction: %d WRITE on var %d with value %d completed\n", trnid, opn->variablenumber, opn->writtenValue, opn->sitenumber) ;
                logString(log_desc) ;
                T[trnid].current_operation = T[trnid].current_operation->opnTM ;
              }
            }
            else {
              int sitenumber, flag_writeStatus = WRITE_CHECK_FOR_COMPLETE ;
              int flag_writePerformed = 0 ;
              for(sitenumber = 1; sitenumber < MAXIMUM_SITES ; sitenumber++ ) {
                if(opn->opnSiteStatus[sitenumber] == OPN_IGNORE || opn->opnSiteStatus[sitenumber] == OPN_COMPLETE) {	
                  continue ;
                }
                else if(opn->opnSiteStatus[sitenumber] == OPN_PENDING) {
                  if(siteInfo[sitenumber].flag_site_available == 0) {
                    opn->opnSiteStatus[sitenumber] = OPN_IGNORE ;	
                    continue ;
                  }
                  else {	
                    opn->waitlistedOpnTicknumber = -1 ;
                    performOperation(opn, sitenumber) ;                                       
                    if(opn->opnSiteStatus[sitenumber] == OPN_REJECTED ) {
                      sprintf(log_desc, "START TRANSACTION MANAGER: Transaction: %d WRITE on variablenumber %d with value %d rejected by site %d\n", trnid, opn->variablenumber, opn->writtenValue, sitenumber) ;
                      logString(log_desc) ;
                      flag_writeStatus = WRITE_FAILED ;
                      break ;
                    }
                    else if(opn->opnSiteStatus[sitenumber] == OPN_BLOCKED) {
                      
                      sprintf(log_desc, "START TRANSACTION MANAGER: Transaction: %d BLOCKED for write on var %d with value %d @ site %d since site could not provide it with the lock\n", trnid, opn->variablenumber, opn->writtenValue, sitenumber) ;
                      logString(log_desc) ;
                      printf("%s", log_desc) ;
                      if(flag_writeStatus != WRITE_PENDING) {
                        flag_writeStatus = WRITE_PENDING ;		
                      }
                    }
                    else if(opn->opnSiteStatus[sitenumber] == OPN_COMPLETE) {
                      
                      if(flag_writePerformed == 0){
                        flag_writePerformed = 1 ;	
                      }
                      if(T[trnid].accessedSites[sitenumber].firstAccessedTick == -1) {        
                        T[trnid].accessedSites[sitenumber].firstAccessedTick = tickNo ;
                      }
                      if(T[trnid].accessedSites[sitenumber].writeAccessed_Flag == 0) {
                        T[trnid].accessedSites[sitenumber].writeAccessed_Flag = 1 ;       
                      }
                      sprintf(log_desc, "START TRANSACTION MANAGER: Transaction: %d WRITE on var %d with value %d at site %d completed\n", trnid, opn->variablenumber, opn->writtenValue, sitenumber) ;
                      logString(log_desc) ;
                    }
                  }
                }
                else if(opn->opnSiteStatus[sitenumber] == OPN_BLOCKED) {
                  if(siteInfo[sitenumber].flag_site_available == 0 ) {  
                    opn->opnSiteStatus[sitenumber] = OPN_IGNORE ;
                  }
                  else {
                    sprintf(log_desc, "START TRANSACTION MANAGER: Transaction: %d still waiting for WRITE on var %d with value %d at site %d\n", trnid, opn->variablenumber, opn->writtenValue, sitenumber) ;
                   
                    printf("%s", log_desc) ;
                    if(flag_writeStatus != WRITE_PENDING) {
                      flag_writeStatus = WRITE_PENDING ;         
                    }
                  }
                }
                else if(opn->opnSiteStatus[sitenumber] == OPN_COMPLETE) {
		  if(T[trnid].accessedSites[sitenumber].firstAccessedTick == -1) {        
		    T[trnid].accessedSites[sitenumber].firstAccessedTick = tickNo ;
		  }
		  if(T[trnid].accessedSites[sitenumber].writeAccessed_Flag == 0) {
		    T[trnid].accessedSites[sitenumber].writeAccessed_Flag = 1 ;       
		  } 
                  if(flag_writePerformed == 0){
		    flag_writePerformed = 1 ;	
		  }
                  sprintf(log_desc,"START TRANSACTION MANAGER: Transaction: %d WRITE on var %d with value %d at site %d completed\n", trnid, opn->variablenumber, opn->writtenValue, sitenumber) ;
                  logString(log_desc) ;
                }
              }
              if(flag_writeStatus == WRITE_CHECK_FOR_COMPLETE) {	
                if(flag_writePerformed == 1) { 
		  T[trnid].current_operation = T[trnid].current_operation->opnTM ;
                  sprintf(log_desc, "START TRANSACTION MANAGER: Transaction: %d WRITE on variablenumber %d with value %d completed at all available sites\n", trnid, opn->variablenumber, opn->writtenValue) ;
                  logString(log_desc) ;
                }
                else {
#ifdef ABORT_SITE_FAIL
                  printf("START TRANSACTION MANAGER: Transaction: %d ABORTED since write on var %d with value %d could not be completed at any of the sites\n", trnid, opn->variablenumberopn->writtenValue) ;
                  abortTransaction(opn) ;
                  T[trnid].trn_Completion_Status = TRN_ABORTED ;
                  T[trnid].current_operation = NULL ;
#endif
#ifdef WAIT_SITE_FAIL
                  if(opn->waitlistedOpnTicknumber == -1) {
                    opn->waitlistedOpnTicknumber = tickNo ;
                  }
                  int i = 1 ;
                  for(i = 1; i < MAXIMUM_SITES; i++) {		
                    opn->opnSiteStatus[i] = OPN_PENDING ;
                  }
                  sprintf(log_desc ,"START TRANSACTION MANAGER: Transaction: %d BLOCKED at write on var %d with value %d since all sites have failed. Retrying on the sites at the next tick..\n", trnid, opn->variablenumber, opn->writtenValue) ;
                  logString(log_desc) ;
#endif
                }
	      } 
              else if(flag_writeStatus == WRITE_FAILED) {
                sprintf(log_desc, "START TRANSACTION MANAGER: Transaction: %d ABORTED since write on variablenumber %d with value to be written %d rejected by site %d\n", trnid, opn->variablenumber, opn->writtenValue, sitenumber) ;
                logString(log_desc) ;
                abortTransaction(opn) ;
                T[trnid].trn_Completion_Status = TRN_ABORTED ;
                T[trnid].current_operation = NULL ;
              }
            }
          }

          else if(opn->opnType == END_OPN ) {
            int sitenumber, flagCommit = 1 ;
	    for(sitenumber = 1; sitenumber < MAXIMUM_SITES && flagCommit == 1 && T[trnid].trnType != RONLY_TRANSACTIONS; sitenumber++) {
	      if(T[trnid].accessedSites[sitenumber].firstAccessedTick != -1) {	
		if(siteInfo[sitenumber].flag_site_available == 0 ) {
		  flagCommit = 0 ;
		  sprintf(log_desc, "START TRANSACTION MANAGER: Transaction %d could not commit and has been ABORTED since site %d has failed\n", trnid, sitenumber) ;
                  logString(log_desc) ;
		  abortTransaction(opn) ;
		  T[trnid].trn_Completion_Status = TRN_ABORTED ;
		  T[trnid].current_operation = NULL ;
		}
		else if(siteInfo[sitenumber].tick_time > T[trnid].accessedSites[sitenumber].firstAccessedTick) {	
                  flagCommit = 0 ;
                  sprintf(log_desc, "START TRANSACTION MANAGER: Transaction %d ABORTED since site %d had failed at some point after the transaction first accessed it\n", trnid, sitenumber) ;
                  logString(log_desc) ;
                  abortTransaction(opn) ; 
		  T[trnid].trn_Completion_Status = TRN_ABORTED ;
                  T[trnid].current_operation = NULL ;
		} 
	      }
	    }
	    if(flagCommit == 1) {
	      sprintf(log_desc, "START TRANSACTION MANAGER: Transaction %d COMMITED @ tick %d\n", trnid, tickNo) ; 
              logString(log_desc) ;
	      for(sitenumber = 1; sitenumber < MAXIMUM_SITES; sitenumber++) {
		if(T[trnid].accessedSites[sitenumber].firstAccessedTick != -1 && siteInfo[sitenumber].flag_site_available == 1) {
		    performOperation(T[trnid].current_operation,sitenumber); 
		} 
	      }
	      T[trnid].current_operation = NULL ; 
              T[trnid].trn_Completion_Status = TRN_COMMITED ;
	    }
          }
        }
      }
      else if(T[trnid].current_operation == NULL && (T[trnid].trn_Completion_Status == -1 && T[trnid].trn_valid_flag == 1)) {
	if(flagPending == 0) {
	  flagPending = 1 ;
        }
        if(T[trnid].inactive_tick_number == -1) {
          T[trnid].inactive_tick_number = tickNo ;
	  sprintf(log_desc, "START TRANSACTION MANAGER: Transaction %d is WAITING for some operation to be received from the input sequence\n", trnid) ;
          logString(log_desc) ;
        }
        else {
	  sprintf(log_desc, "START TRANSACTION MANAGER: Transaction %d still WAITING for a new operation from the input for over %d ticks\n", trnid, tickNo - T[trnid].inactive_tick_number) ;
          logString(log_desc) ;
        }
      }
    }
    Sleep_ms(SLEEP_DURATION_MS) ; 
    tickNo++ ;
  }
}

void abortTransaction(struct operation *opn) {
  struct operation abort_opn ;
  int sitenumber ;
  int trnid = opn->trnid ;
  memcpy(&abort_opn, opn, sizeof(struct operation)) ;		// CHECK HERE
  abort_opn.opnType = ABORT_OPN ;

  for(sitenumber = 1; sitenumber < MAXIMUM_SITES; sitenumber++) {
    abort_opn.sitenumber = sitenumber ; 
    abort_opn.opnSite = NULL ; 
    if(T[trnid].accessedSites[sitenumber].firstAccessedTick != -1 && siteInfo[sitenumber].flag_site_available == 1) {
      performOperation(&abort_opn, sitenumber) ;
    }
  }
  return ; 
}

/****************************************************************************************************************
						Function parseInput Starts								
*************************************************************************************************************************/
int parseInputFile(char *inputfile) 
{
  int status = 0;
  int time_stamp = 0 ;
  int i;
  int trx_id;
  FILE *fp;

  T[OTHER_TRNID].trnType = OTHER_TRANSACTIONS;
  
  fp = fopen(inputfile, "r") ;
  if(fp == NULL) {
    printf("parseInputFile: failed to open file") ;
    return -1 ;
  }
  //printf("file opened successfully\n");
  while(!feof(fp)) 
  {
    
    char opn_data[100]; 
    char *temp_opn;
    char *temp_tok;
    char *temp_tok1;
    char temp_token[100];
    const char s[2] = ";";
    memset(opn_data,0,100) ;
    memset(temp_token,0,100) ;
    fscanf(fp, "%s", opn_data) ;  
    if(opn_data[0] == '/' || opn_data[0] == '#') 
    {
      fgets(opn_data,100,fp);
      continue;
    }
    
    temp_tok1 = strstr(opn_data,s); 
    
    if(temp_tok1 != NULL)
    {
    temp_tok = strtok(opn_data,";");
    while(temp_tok != NULL)
    {
      
      memset(temp_token,0,100);
      strncpy(temp_token,temp_tok,100); 
       

      if(strncmp(temp_token,"begin", strlen("begin")) == 0
      || strncmp(temp_token,"R", strlen("R")) == 0
      || strncmp(temp_token,"W", strlen("W")) == 0
      || strncmp(temp_token,"fail", strlen("fail")) == 0
      || strncmp(temp_token,"recover", strlen("recover")) == 0
      || strncmp(temp_token,"dump", strlen("dump")) == 0
      || strncmp(temp_token,"end", strlen("end")) == 0
      || strncmp(temp_token,"querystate", strlen("querystate")) == 0)
      {
        
        status = storeOperation(temp_token, time_stamp);
        if(status == -1)
        {
          printf("\n parseInputFile: StoreOpn failed for operation %s",opn_data);
          return -1;
        } 
      } 
      temp_tok = strtok(NULL,";");
    }
    time_stamp++; 
    }
    else
    {
    if(strncmp(opn_data,"begin", strlen("begin")) == 0 
    || strncmp(opn_data,"R", strlen("R")) == 0
    || strncmp(opn_data,"W", strlen("W")) == 0
    || strncmp(opn_data,"fail", strlen("fail")) == 0
    || strncmp(opn_data,"recover", strlen("recover")) == 0
    || strncmp(opn_data,"dump", strlen("dump")) == 0
    || strncmp(opn_data,"end", strlen("end")) == 0
    || strncmp(opn_data, "querystate", strlen("querystate")) == 0)
    {
      
      status = storeOperation(opn_data, time_stamp);
      if(status == -1)
      {
        printf("\n parseInputFile: AddOpn failed for operation %s",opn_data);
        return -1;
      }
      
      temp_opn = strstr(opn_data, ";");
     // printf("\n temp_opn = %s",temp_opn);
      
      if(temp_opn == NULL)
      {
        time_stamp++;
      }
    } 
    }
  }
  return 0;
}
int storeOperation(char *operationString, int operationtimestamp) {
  int trnid = -1, ret ;
  char *temp ;
  struct operation *opn = (struct operation *)malloc(sizeof(struct operation )) ;
  if(opn == NULL) {
    printf("storeOperation: Fatal Error. malloc for struct operation failed. Error: %s\n", strerror(errno)) ;
    return -1 ;
  }
  if(strncmp(operationString,"beginRO", strlen("beginRO")) == 0) {	
    temp = operationString ;
    while(!isdigit(*temp))
     temp++ ;
    trnid = atoi(temp) ;

    int trnType = RONLY_TRANSACTIONS ;
    ret = createNewTransaction(trnid, trnType, operationtimestamp) ;
    if(ret == -1) {
      printf("storeOperation: createNewTransaction returns error for new transaction trnid %d\n", trnid) ;
      return -1 ;
    }
  }
  else if(strncmp(operationString,"begin", strlen("begin")) == 0) {	
    temp = operationString ;
    while(!isdigit(*temp))
     temp++ ;
    trnid = atoi(temp) ;
    //printf("New transaction %d\n", trnid) ;
    int trnType = READ_WRITE_TRANSACTIONS ;
    ret = createNewTransaction(trnid, trnType, operationtimestamp) ;
    if(ret == -1) {
      printf("storeOperation: createNewTransaction returns error for new transaction trnid %d\n", trnid) ;
      return -1 ;
    }
  }
  else if(strncmp(operationString,"end", strlen("end")) == 0) {	
    temp = operationString ;
    while(!isdigit(*temp))
     temp++ ;
    trnid = atoi(temp) ;
    int variablenumber = -1, writtenValue = -1, sitenumber = -1 ;
    ret = prepareOperationNode(trnid, END_OPN, variablenumber, writtenValue, sitenumber, operationtimestamp, opn) ;
    if(ret == -1) {
      printf("storeOperation: prepareOperationNode returns error for end operation\n") ;
      return -1 ;
    }
    addOperationToTransactionQueue(trnid, opn) ;


    
  }
  else if(strncmp(operationString,"dump", strlen("dump")) == 0) {
    int variablenumber = -1, sitenumber ;
    trnid = OTHER_TRNID ;
    temp = strstr(operationString, "(") ;
    if(temp == NULL) {
     printf("storeOperation: Could not parse dump operation %s\n", operationString) ;
     return -1 ;
    }
    while(*temp != ')' && !isalpha(*temp) && !isdigit(*temp))
     temp++ ;


    if(*temp == ')') {		
     variablenumber = ALL_VARIABLES ;
     sitenumber = ALL_SITES ;
    }

    else if(isdigit(*temp)) {		
     sitenumber = atoi(temp) ;
     variablenumber = ALL_VARIABLES ;

     
    }
    else if(isalpha(*temp)) {
     while(!isdigit(*temp))
      temp++ ;
     variablenumber = atoi(temp) ;
     sitenumber = ALL_SITES ;

     
    }
    int writtenValue = -1 ;
    ret = prepareOperationNode(trnid, DUMP_OPN, variablenumber, writtenValue, sitenumber, operationtimestamp, opn) ;
    if(ret == -1) {
      printf("storeOperation: prepareOperationNode returns error for dump operation\n") ;
      return -1 ;
    }

    addOperationToTransactionQueue(trnid, opn) ;
  }
  else if(strncmp(operationString,"fail", strlen("fail")) == 0) {
    trnid = OTHER_TRNID ;
    temp = strstr(operationString, "(") ;
    if(temp == NULL) {
     printf("storeOperation: Could not parse fail operation %s\n", operationString) ;
     return -1 ;
    }
    while(!isdigit(*temp))
     temp++ ;
    int sitenumber = atoi(temp) ;
    int variablenumber = -1, writtenValue = -1 ;
    ret = prepareOperationNode(trnid, FAIL_OPN, variablenumber, writtenValue, sitenumber, operationtimestamp, opn) ;
    if(ret == -1) {
      printf("storeOperation: prepareOperationNode returns error for fail operation\n") ;
      return -1 ;
    }
    addOperationToTransactionQueue(trnid, opn) ;
  }
  else if(strncmp(operationString,"recover", strlen("recover")) == 0) {
    trnid = OTHER_TRNID ;
    temp = strstr(operationString, "(") ;
    if(temp == NULL) {
     printf("storeOperation: Could not parse recover operation %s\n", operationString) ;
     return -1 ;
    }
    while(!isdigit(*temp))
     temp++ ;
    int sitenumber = atoi(temp) ;

    
    int variablenumber = -1, writtenValue = -1 ;
    ret = prepareOperationNode(trnid, RECOVER_OPN, variablenumber, writtenValue, sitenumber, operationtimestamp, opn) ;
    if(ret == -1) {
      printf("storeOperation: prepareOperationNode returns error for recover operation\n") ;
      return -1 ;
    }
    addOperationToTransactionQueue(trnid, opn) ;
  }
  else if(strncmp(operationString,"querystate", strlen("querystate")) == 0) {
    trnid = OTHER_TRNID ;
    int sitenumber = ALL_SITES, variablenumber = ALL_VARIABLES, writtenValue = -1 ;
    ret = prepareOperationNode(trnid, QUERY_STATE_OPN, variablenumber, writtenValue, sitenumber, operationtimestamp, opn) ;
    if(ret == -1) {
      printf("storeOperation: prepareOperationNode returns error for new operation\n") ;
      return -1 ;
    }
    addOperationToTransactionQueue(trnid, opn) ;
  }
  else if(strncmp(operationString,"R", strlen("R")) == 0) {	
    temp = operationString ;
    while(!isdigit(*temp))
     temp++ ;
    trnid = atoi(temp) ;
    while(isdigit(*temp))
     temp++ ;
    while(!isdigit(*temp))
     temp++ ;
    int variablenumber = atoi(temp) ;
    int writtenValue = -1, sitenumber = -1 ;
    ret = prepareOperationNode(trnid, READ_OPN, variablenumber, writtenValue, sitenumber, operationtimestamp, opn) ;
    if(ret == -1) {
      printf("storeOperation: prepareOperationNode returns error for new operation\n") ;
      return -1 ;
    }
    addOperationToTransactionQueue(trnid, opn) ;


   }
  else if(strncmp(operationString,"W", strlen("W")) == 0) {	
    temp = operationString ;
    while(!isdigit(*temp))
     temp++ ;
    trnid = atoi(temp) ;

    while(isdigit(*temp))
     temp++ ;
    while(!isdigit(*temp))
     temp++ ;
    int variablenumber = atoi(temp) ;

    while(isdigit(*temp))
     temp++ ;
    while(!isdigit(*temp) && *temp != '-')
     temp++ ;
    int writtenValue = atoi(temp), sitenumber = -1 ;
    ret = prepareOperationNode(trnid, WRITE_OPN, variablenumber, writtenValue, sitenumber, operationtimestamp, opn) ;
    if(ret == -1) {
      printf("storeOperation: prepareOperationNode returns error for new operation\n") ;
      return -1 ;
    }
    addOperationToTransactionQueue(trnid, opn) ;

    
  }
  return  0 ;
}

//A new transaction entry is created
int createNewTransaction(int trnid, int trnType, int timestamp) {
  if(trnid >= MAXIMUM_TRANSACTIONS ) {	//Transaction Manager can't support transaction ids greater than MAXIMUM_TRANSACTIONS
    printf("createNewTransaction: Transaction %d exceeds Max limit %d\n", trnid, MAXIMUM_TRANSACTIONS) ;
    return -1 ;
  }
  if(T[trnid].timestamp != -1 ) {
    printf("createNewTransaction: Error duplication of trnid %d in input transaction sequence\n", trnid) ;
    return -1 ;
  }
  T[trnid].timestamp = timestamp ;
  T[trnid].trnType = trnType ;
  T[trnid].trn_valid_flag = 1 ;
  return 0 ;
}

void addOperationToTransactionQueue(int trnid, struct operation *opn) {
  if(T[trnid].first_operation == NULL) {	//This is the first operation assigned to that transaction
    T[trnid].first_operation = T[trnid].last_operation = T[trnid].current_operation = opn ;
  }
  else {
    T[trnid].last_operation->opnTM = opn ;
    T[trnid].last_operation = opn ;
  }
  return  ;
}

int prepareOperationNode(int trnid, int opnType, int variablenumber, int writtenValue, int sitenumber, int operationtimestamp, struct operation *opn) {
  opn->trnid = trnid ;
  opn->opnType = opnType ;
  opn->opnTimestamp = operationtimestamp ;
  opn->opnTM = NULL ;
  opn->variablenumber = variablenumber ;
  opn->writtenValue = writtenValue ;
  opn->sitenumber = sitenumber ;
  opn->waitlistedOpnTicknumber = -1 ;
  int site_No ;
  for(site_No = 1 ; site_No < MAXIMUM_SITES; site_No++) {
    opn->opnSiteStatus[site_No] = OPN_PENDING ;
  }
  
  if(T[trnid].timestamp == -1 && trnid == OTHER_TRNID) {
    T[trnid].timestamp = operationtimestamp ;
  }
  opn->trnType = T[trnid].trnType ;
  opn->trnTimestamp = T[trnid].timestamp ;

  return 0 ;
}






void initializeTransactionManager() {
  //Initialize all transaction structures
  int trnid, sitenumber ;
  for(trnid = 0; trnid < MAXIMUM_TRANSACTIONS; trnid++) {
    T[trnid].timestamp = -1 ;
    T[trnid].trnType = -1 ;
    T[trnid].trn_Completion_Status = -1 ;
    T[trnid].inactive_tick_number = -1 ;
    T[trnid].trn_valid_flag = 0 ;
    T[trnid].first_operation = T[trnid].last_operation = T[trnid].current_operation = NULL ;
    for(sitenumber = 0; sitenumber < MAXIMUM_SITES; sitenumber++)  {
      T[trnid].accessedSites[sitenumber].firstAccessedTick = -1 ;
      T[trnid].accessedSites[sitenumber].writeAccessed_Flag = 0 ; 
    }
  }

  //Initialize variable-site mapping
  int variablenumber ;
  for(variablenumber = 1; variablenumber < MAXIMUM_VARIABLES ; variablenumber++)  {
    if(variablenumber % 2 == 1) {	//if the variable is odd, use the given data in problem statement to find the sitenumber @ which it will be stored
      sitenumber = (variablenumber % 10) + 1 ;
      siteInfo[sitenumber].flag_variablenumber[variablenumber] = 1 ;
    }
    else {	//Even variables are present at all sites
      for(sitenumber = 1; sitenumber < MAXIMUM_SITES ; sitenumber++ ) {
        siteInfo[sitenumber].flag_variablenumber[variablenumber] = 1 ;
      }
    }
  }

  for(sitenumber = 1; sitenumber < MAXIMUM_SITES; sitenumber++) {	//All sites are assumed to be up initially
    siteInfo[sitenumber].flag_site_available = 1 ;
    siteInfo[sitenumber].tick_time = 0 ;		//All sites are assumed to be up from the 0th tick
  }
}


void Sleep_ms(int time_ms) {
  struct timeval tv ;
  tv.tv_sec = 0 ;
  tv.tv_usec = time_ms * 1000 ;
  select(0, NULL , NULL, NULL, &tv) ;
  return ;
}
