#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>


typedef struct
{
	unsigned int bit:2;
}TwoBits;


struct BTB
{
	unsigned int size;
	unsigned int assoc;
	unsigned int blocksize;
	unsigned int numOfSets;

	unsigned int* tagArray;
	int* LRUCounter;

	unsigned int branchCounter;
	unsigned int BTBMissButBranchTaken;
	unsigned int noOfPredictionsFromBTB;
};



void extractIndexPredictionTable(unsigned int addressInInt, int m2BimodalBits, unsigned int* indexLocation);
int biModalPredictorFunction(int m2Bimodal, int btbSize, int btbAssoc, char *traceFileName, BTB *btbBuffer);
int gsharePredictorFunction(int m1GShare, int nBitsGlobalBHR, int btbSize, int btbAssoc, char *traceFileName, BTB *btbBuffer);
int hybridPredictorFunction(int m1GShare, int nBitsGlobalBHR, int m2Bimodal, int kBitsChooser, int btbSize, int btbAssoc, char *traceFileName, BTB *btbBuffer);

void extractAddressParams(unsigned int addressInInt, struct BTB* btbBuffer, unsigned int* indexLocation, unsigned int* tagAddress);


void LRUForHit(struct BTB* btbBuffer, unsigned int indexLocation, unsigned int tagLocation);
void LRUForMiss(struct BTB* btbBuffer, unsigned int indexLocation, unsigned int* tagLocation);
int readFromBTB(struct BTB* btbBuffer, unsigned int addressInInt);



int main(int agrc, char* argv[])
{
	int m2Bimodal = 0, m1GShare = 0,nBitsGlobalBHR = 0, kBitsChooser = 0;
	int btbSize = 0, btbAssoc = 0, noOfTagEntries = 0;

	char *branchPredictorType, *traceFileName, type;
	
	struct BTB *btbBuffer = (struct BTB*)malloc(sizeof(struct BTB));

	
	branchPredictorType = argv[1];
	if( strcmp(branchPredictorType, "bimodal") == 0 )
	{
		type = 'B';
		m2Bimodal = atoi(argv[2]);
		btbSize = atoi(argv[3]);
		btbAssoc = atoi(argv[4]);
		traceFileName = argv[5];

		printf("COMMAND\n");
		printf("./sim bimodal %d %d %d %s\n", m2Bimodal, btbSize, btbAssoc, traceFileName);
		printf("OUTPUT\n");
	}
	
	if( strcmp(branchPredictorType, "gshare") == 0 )
	{
		type = 'G';
		m1GShare = atoi(argv[2]);
		nBitsGlobalBHR = atoi(argv[3]);
		btbSize = atoi(argv[4]);
		btbAssoc = atoi(argv[5]);
		traceFileName = argv[6];

		printf("COMMAND\n");
		printf("./sim gshare %d %d %d %d %s\n", m1GShare, nBitsGlobalBHR, btbSize, btbAssoc, traceFileName);
		printf("OUTPUT\n");
	}

	if( strcmp(branchPredictorType, "hybrid") == 0 )
	{
		type = 'H';
		kBitsChooser = atoi(argv[2]);
		m1GShare = atoi(argv[3]);
		nBitsGlobalBHR = atoi(argv[4]);
		m2Bimodal = atoi(argv[5]);
		btbSize = atoi(argv[6]);
		btbAssoc = atoi(argv[7]);
		traceFileName = argv[8];

		printf("COMMAND\n");
		printf("./sim hybrid %d %d %d %d %d %d %s\n", kBitsChooser, m1GShare, nBitsGlobalBHR, m2Bimodal, btbSize, btbAssoc, traceFileName);
		printf("OUTPUT\n");
	}
	

	
	
	if( btbSize == 0 )
	{
		btbBuffer->size = btbSize;
	}
	else
	{

		btbBuffer->size = btbSize;
		btbBuffer->blocksize = 4;
		btbBuffer->assoc = btbAssoc;
		

		btbBuffer->numOfSets = (btbBuffer->size/(btbBuffer->blocksize*btbBuffer->assoc));

		noOfTagEntries = btbBuffer->numOfSets*btbBuffer->assoc;

		btbBuffer->tagArray = (unsigned int*)malloc( (noOfTagEntries*sizeof(unsigned int)) );
	btbBuffer->LRUCounter = (int*)malloc( (noOfTagEntries*sizeof(int)) );


		memset( btbBuffer->tagArray, 0, (sizeof(btbBuffer->tagArray[0])*noOfTagEntries) );
		memset( btbBuffer->LRUCounter, 0, (sizeof(btbBuffer->LRUCounter[0])*noOfTagEntries) );


		btbBuffer->branchCounter = 0;
		btbBuffer->noOfPredictionsFromBTB = 0;
		btbBuffer->BTBMissButBranchTaken = 0;
	}




	if( type == 'B' )
	{

		biModalPredictorFunction(m2Bimodal, btbSize, btbAssoc, traceFileName, btbBuffer);
	}
	
	if( type == 'G' )
	{
		
		gsharePredictorFunction(m1GShare, nBitsGlobalBHR, btbSize, btbAssoc, traceFileName, btbBuffer);
	}

	if(type == 'H' )
	{

		hybridPredictorFunction(m1GShare, nBitsGlobalBHR, m2Bimodal, kBitsChooser, btbSize, btbAssoc, traceFileName, btbBuffer);
	}

	return 0;
	
}



void extractIndexPredictionTable(unsigned int addressInInt, int m2BimodalBits, unsigned int* indexLocation)
{
	int tempIndexNo = 0, i=0;


	*indexLocation = addressInInt>>2;

	for( i=0; i<m2BimodalBits; i++)
	{
		tempIndexNo = ( 1 | tempIndexNo<<1 );
	}

	*indexLocation = ( *indexLocation & tempIndexNo );
}





void extractAddressParams(unsigned int addressInInt, struct BTB* btbBuffer, unsigned int* indexLocation, unsigned int* tagAddress)
{
	int noOfBlockBits = 0, noOfIndexBits = 0, tempIndexNo = 0, i=0;
	
	noOfBlockBits = log2(btbBuffer->blocksize);
	noOfIndexBits = log2(btbBuffer->numOfSets);

	*indexLocation = addressInInt>>noOfBlockBits;

	for( i=0; i<noOfIndexBits; i++)
	{
		tempIndexNo = ( 1 | tempIndexNo<<1 );
	}

	*indexLocation = ( *indexLocation & tempIndexNo );
	*tagAddress = addressInInt>>(noOfBlockBits + noOfIndexBits);
}








void LRUForHit(struct BTB* btbBuffer, unsigned int indexLocation, unsigned int tagLocation)
{
	int i = 0;

	for( i=0; i< (int)btbBuffer->assoc; i++)
	{
		if( (int)btbBuffer->LRUCounter[indexLocation + (i*btbBuffer->numOfSets)] < (int)btbBuffer->LRUCounter[tagLocation] )	
		{
			btbBuffer->LRUCounter[indexLocation + (i*btbBuffer->numOfSets)] = ((int)btbBuffer->LRUCounter[indexLocation + (i*btbBuffer->numOfSets)]) + 1;
		}
	}
	
	btbBuffer->LRUCounter[tagLocation] = 0;
}



void LRUForMiss(struct BTB* btbBuffer, unsigned int indexLocation, unsigned int* tagLocation)
{
	unsigned int i = 0;
	int max = -1;
	*tagLocation = 0;
	
	for( i=0; i<btbBuffer->assoc; i++)
	{
		if( (int)btbBuffer->LRUCounter[indexLocation + (i*btbBuffer->numOfSets)] > (int)max )
		{
			max = btbBuffer->LRUCounter[indexLocation + (i*btbBuffer->numOfSets)];
			*tagLocation = ( indexLocation + (i*btbBuffer->numOfSets) );
		}
	}


	for( i=0; i<btbBuffer->assoc; i++)
	{
		btbBuffer->LRUCounter[indexLocation + (i*btbBuffer->numOfSets)] = ((int)btbBuffer->LRUCounter[indexLocation + (i*btbBuffer->numOfSets)]) + 1;
	}

	btbBuffer->LRUCounter[*tagLocation] = 0;

}




int readFromBTB(struct BTB* btbBuffer, unsigned int addressInInt)
{
	int i=0;
	unsigned int indexLocation = 0, tagAddress = 0, tagLocation = 0;

	extractAddressParams(addressInInt, btbBuffer, &indexLocation, &tagAddress);

	btbBuffer->branchCounter +=1;

	for( i=0; i< (int)btbBuffer->assoc; i++)
	{
		if( btbBuffer->tagArray[indexLocation + (i*btbBuffer->numOfSets)] == tagAddress )	//Checking Tag Entries
		{
				
			LRUForHit(btbBuffer, indexLocation, ( indexLocation + (i*btbBuffer->numOfSets) ) );
								
			return(1);
		}
		
	}


	LRUForMiss(btbBuffer, indexLocation, &tagLocation);
	btbBuffer->LRUCounter[tagLocation] = 0;
	
	btbBuffer->tagArray[tagLocation] = tagAddress;

	return(0);
}




int biModalPredictorFunction(int m2Bimodal, int btbSize, int btbAssoc, char *traceFileName, BTB *btbBuffer)
{

	FILE 	*traceFile;
	int 	noOfEntriesInPredictorTable = 0, i = 0, getVal = 0;
	char 	readAddress[100], *branchActuallyTaken, *hexAddress;
	unsigned int addressInInt = 0, indexLocation = 0;
	unsigned int noOfPredictions = 0, noOfMisPredictions = 0;

	if( (traceFile = fopen(traceFileName, "r" ) ) == NULL )
	{
		printf("\nTrace File cannot be opened");
		return(0);
	}
	


	noOfEntriesInPredictorTable = pow(2, m2Bimodal);
	TwoBits predictorTable[noOfEntriesInPredictorTable];
		
	for(i=0;i<noOfEntriesInPredictorTable;i++)
	{
		predictorTable[i].bit = 2;
	}
		
		
	while( (fgets(readAddress, 100, traceFile) ) != NULL )
	{
		hexAddress = strtok(readAddress, " ");
		branchActuallyTaken = strtok(NULL, "\n");
		addressInInt = strtoll(hexAddress, NULL, 16);
		noOfPredictions++;


		if( btbBuffer->size != 0)
		{
			
			getVal = readFromBTB(btbBuffer, addressInInt);
		}




		if( (btbBuffer->size == 0) || (getVal == 1) )
		{
			btbBuffer->noOfPredictionsFromBTB++;
			extractIndexPredictionTable(addressInInt, m2Bimodal, &indexLocation);
				
				
			if( predictorTable[indexLocation].bit >= 2 )
			{
				//branch predicted taken
				if( (strcmp(branchActuallyTaken, "t") == 0) && predictorTable[indexLocation].bit !=3 )
				{
					predictorTable[indexLocation].bit = predictorTable[indexLocation].bit + 1;
				}
				else if( (strcmp(branchActuallyTaken, "n") == 0) && predictorTable[indexLocation].bit != 0 )
				{
					predictorTable[indexLocation].bit = predictorTable[indexLocation].bit - 1;
					noOfMisPredictions++;
				}
			}
			else
			{
				//branch predicted not-taken
				
				if( (strcmp(branchActuallyTaken, "t") == 0) && predictorTable[indexLocation].bit !=3 )
				{
					predictorTable[indexLocation].bit = predictorTable[indexLocation].bit + 1;
					noOfMisPredictions++;
				}
				else if( (strcmp(branchActuallyTaken, "n") == 0) && predictorTable[indexLocation].bit != 0 )
				{
					predictorTable[indexLocation].bit = predictorTable[indexLocation].bit - 1;
				}
			}
			
		}


		if( (btbBuffer->size != 0) && (getVal == 0) )
		{
			//branch is not at all or predicted not-taken
			//BTB miss
			//take account of those branches taken
			if( (strcmp(branchActuallyTaken, "t") == 0) )
				btbBuffer->BTBMissButBranchTaken++;

		}	
	}


	//BTB contents
	int j= 0;
	if( btbBuffer->size!=0)
	{
		printf("size of BTB:	 %d\n", btbBuffer->size);
		printf("number of branches:		%d\n", btbBuffer->branchCounter);
		printf("number of predictions from branch predictor:	%d\n", btbBuffer->noOfPredictionsFromBTB);
		printf("number of mispredictions from branch predictor: %d\n", noOfMisPredictions);
		printf("number of branches miss in BTB and taken:	 %d\n", btbBuffer->BTBMissButBranchTaken);
		printf("total mispredictions:	%d\n", (noOfMisPredictions+btbBuffer->BTBMissButBranchTaken));

		double missrate = ((double)(noOfMisPredictions+btbBuffer->BTBMissButBranchTaken) / btbBuffer->branchCounter)*100;
		printf("misprediction rate:	%0.2f%%\n", missrate);

		printf("\nFINAL BTB CONTENTS\n");
		for( i=0; i<(int)btbBuffer->numOfSets; i++)
		{
			//sort L1 cache based on LRU counter
			printf("set %-4d:	", i);
			for( j=0; j<(int)btbBuffer->assoc; j++)
			{
				printf("%-10x",btbBuffer->tagArray[i + (j*btbBuffer->numOfSets)]);
			}
			printf("\n");
		}

		printf("\nFINAL BIMODAL CONTENTS\n");
		for(int i=0;i<noOfEntriesInPredictorTable;i++)
		{
			printf("%0d%6d\n", i, predictorTable[i].bit );
		}
	}
	else
	{
		
		printf("number of predictions: %d\n", noOfPredictions);
		printf("number of mispredictions: %d\n", noOfMisPredictions);
		double missrate = ((double)noOfMisPredictions / noOfPredictions)*100;
		printf("misprediction rate: %0.2f%%\n", missrate);

		printf("FINAL BIMODAL CONTENTS\n");
		for(int i=0;i<noOfEntriesInPredictorTable;i++)
		{
			printf("%d	%d\n", i, predictorTable[i].bit );
		}
	
	}

	return(1);
}





int gsharePredictorFunction(int m1GShare, int nBitsGlobalBHR, int btbSize, int btbAssoc, char *traceFileName, BTB *btbBuffer)
{
	
	FILE 	*traceFile;
	int 	noOfEntriesInPredictorTable = 0, i = 0, getVal = 0;
	char 	readAddress[100], *branchActuallyTaken, *hexAddress;
	unsigned int addressInInt = 0, indexLocation = 0, globalBHR = 0;
	unsigned int constructN_BHR_Bits = 0, constructN_Bits_From_IndexLoc = 0;
	unsigned int construct_Low_Order_Index_Bits = 0;
	unsigned int noOfPredictions = 0, noOfMisPredictions = 0;

	
	if( (traceFile = fopen(traceFileName, "r" ) ) == NULL )
	{
		printf("\nTrace File cannot be opened");
		return(0);
	}

	
	
	
	noOfEntriesInPredictorTable = pow(2, m1GShare);
	TwoBits predictorTable[noOfEntriesInPredictorTable];
	
	for(i=0;i<noOfEntriesInPredictorTable;i++)
	{
		predictorTable[i].bit = 2;
	}
		
	while( (fgets(readAddress, 100, traceFile) ) != NULL )
	{
		hexAddress = strtok(readAddress, " ");
		branchActuallyTaken = strtok(NULL, "\n");
		addressInInt = strtoll(hexAddress, NULL, 16);
		
		noOfPredictions++;

		if( btbBuffer->size != 0)
		{
			
			getVal = readFromBTB(btbBuffer, addressInInt);
		}


		if( (btbBuffer->size == 0) || (getVal == 1) )
		{
			btbBuffer->noOfPredictionsFromBTB++;
			extractIndexPredictionTable(addressInInt, m1GShare, &indexLocation);
			
			constructN_BHR_Bits = 0;
			constructN_Bits_From_IndexLoc = 0;
			
			//get nBitsGlobalBHR bits from BHR
			for( i=0; i<nBitsGlobalBHR; i++)
			{
				constructN_BHR_Bits = ( 1 | constructN_BHR_Bits<<1 );
			}
			globalBHR = ( globalBHR & constructN_BHR_Bits );
			

			//get the uppermost nBitsGlobalBHR bits from indexLocation/gshare
			constructN_Bits_From_IndexLoc = (indexLocation>>(m1GShare-nBitsGlobalBHR));
			

			//XOR globalBHR and constructN_Bits_From_IndexLoc
			constructN_Bits_From_IndexLoc = (globalBHR^constructN_Bits_From_IndexLoc);
			

			construct_Low_Order_Index_Bits = 0;
			//after XOR extract lower m bits from indexLocation and paste it with higher order n bits of XOR results
			for( i=0; i<(m1GShare-nBitsGlobalBHR); i++)
			{
				construct_Low_Order_Index_Bits = ( 1 | construct_Low_Order_Index_Bits<<1 );
			}
			construct_Low_Order_Index_Bits = (indexLocation & construct_Low_Order_Index_Bits);
			

			//Shift N bits got from XOR'ed to left by (n-m) bits, so that lower m bits can be pasted with it
			constructN_Bits_From_IndexLoc = ( constructN_Bits_From_IndexLoc<<(m1GShare-nBitsGlobalBHR) );
			
			
			//Final Index Location
			indexLocation = (constructN_Bits_From_IndexLoc|construct_Low_Order_Index_Bits);
			
				
			if( predictorTable[indexLocation].bit >= 2 )
			{
				//branch predicted taken
				if( (strcmp(branchActuallyTaken, "t") == 0) && predictorTable[indexLocation].bit !=3 )
				{
					predictorTable[indexLocation].bit = predictorTable[indexLocation].bit + 1;
				}
				else if( (strcmp(branchActuallyTaken, "n") == 0) && predictorTable[indexLocation].bit != 0 )
				{
					predictorTable[indexLocation].bit = predictorTable[indexLocation].bit - 1;
					noOfMisPredictions++;
				}
			}
			else
			{
				//branch predicted not-taken
				if( (strcmp(branchActuallyTaken, "t") == 0) && predictorTable[indexLocation].bit !=3 )
				{
					predictorTable[indexLocation].bit = predictorTable[indexLocation].bit + 1;
					noOfMisPredictions++;
				}
				else if( (strcmp(branchActuallyTaken, "n") == 0) && predictorTable[indexLocation].bit != 0 )
				{
					predictorTable[indexLocation].bit = predictorTable[indexLocation].bit - 1;
				}
			}


			//update the BHR by shifting right
			globalBHR = globalBHR>>1;

			constructN_BHR_Bits = 0;
			constructN_Bits_From_IndexLoc = 1;
			for( i=0; i<(nBitsGlobalBHR-1); i++)
			{
				constructN_BHR_Bits = ( 1 | constructN_BHR_Bits<<1 );
				constructN_Bits_From_IndexLoc = ( 0 | constructN_Bits_From_IndexLoc<<1 );
			}


			//At this time I have (n-1) bits BHR with nth position as 0.
			globalBHR = ( globalBHR & constructN_BHR_Bits );
				


			if( (strcmp(branchActuallyTaken, "t") == 0) )
			{
				globalBHR = ( constructN_Bits_From_IndexLoc| globalBHR );
			}		
		}


		if( (btbBuffer->size != 0) && (getVal == 0) )
		{
			//branch is not at all or predicted not-taken
			//BTB miss
			//take account of those branches taken
			if( (strcmp(branchActuallyTaken, "t") == 0) )
				btbBuffer->BTBMissButBranchTaken++;

		}
	}
		
	

	//BTB contents
	int j= 0;
	if( btbBuffer->size!=0)
	{
		printf("size of BTB:	 %d\n", btbBuffer->size);
		printf("number of branches:		%d\n", btbBuffer->branchCounter);
		printf("number of predictions from branch predictor:	%d\n", btbBuffer->noOfPredictionsFromBTB);
		printf("number of mispredictions from branch predictor: %d\n", noOfMisPredictions);
		printf("number of branches miss in BTB and taken:	 %d\n", btbBuffer->BTBMissButBranchTaken);
		printf("total mispredictions:	%d\n", (noOfMisPredictions+btbBuffer->BTBMissButBranchTaken));

		double missrate = ((double)(noOfMisPredictions+btbBuffer->BTBMissButBranchTaken) / btbBuffer->branchCounter)*100;
		printf("misprediction rate:	%0.2f%%\n", missrate);

		printf("\nFINAL BTB CONTENTS\n");
		for( i=0; i<(int)btbBuffer->numOfSets; i++)
		{
			//sort L1 cache based on LRU counter
			printf("set %-4d:	", i);
			for( j=0; j<(int)btbBuffer->assoc; j++)
			{
				printf("%-10x",btbBuffer->tagArray[i + (j*btbBuffer->numOfSets)]);
			}
			printf("\n");
		}

		printf("\nFINAL GSHARE CONTENTS\n");
		for(int i=0;i<noOfEntriesInPredictorTable;i++)
		{
			printf("%d	%d\n", i, predictorTable[i].bit );
		}
	}
	else
	{
		
		printf("number of predictions: %d\n", noOfPredictions);
		printf("number of mispredictions: %d\n", noOfMisPredictions);
		double missrate = ((double)noOfMisPredictions / noOfPredictions)*100;
		printf("misprediction rate: %0.2f%%\n", missrate);

		printf("FINAL GSHARE CONTENTS\n");
		for(int i=0;i<noOfEntriesInPredictorTable;i++)
		{
			printf("%d	%d\n", i, predictorTable[i].bit );
		}
	
	}
	

	return(1);
}





int hybridPredictorFunction(int m1GShare, int nBitsGlobalBHR, int m2Bimodal, int kBitsChooser, int btbSize, int btbAssoc, char *traceFileName, BTB *btbBuffer)
{
	
	FILE 	*traceFile;
	int 	bimodalPredictorTableEntries = 0, i = 0, gsharePredictorTableEntries = 0;
	int 	chooserTableEntries = 0, bimodalTrueFalse = 0, gshareTrueFalse = 0, getVal = 0;
	char 	readAddress[100], *branchActuallyTaken, *hexAddress;
	unsigned int addressInInt = 0, indexLocationBimodal = 0, chooserIndexLocation = 0;;
	unsigned int noOfPredictions = 0, noOfMisPredictions = 0;

	unsigned int indexLocationGshare = 0, globalBHR = 0;
	unsigned int constructN_BHR_Bits = 0, constructN_Bits_From_IndexLoc = 0;
	unsigned int construct_Low_Order_Index_Bits = 0;


	if( (traceFile = fopen(traceFileName, "r" ) ) == NULL )
	{
		printf("\nTrace File cannot be opened");
		return(0);
	}

	
	bimodalPredictorTableEntries = pow(2, m2Bimodal);
	TwoBits predictorTableBimodal[bimodalPredictorTableEntries];

	gsharePredictorTableEntries = pow(2, m1GShare);
	TwoBits predictorTableGshare[gsharePredictorTableEntries];

	chooserTableEntries = pow(2, kBitsChooser);
	TwoBits chooserPredictorTable[chooserTableEntries];

		
	for(i=0;i<bimodalPredictorTableEntries;i++)
	{
		predictorTableBimodal[i].bit = 2;
	}

	for(i=0;i<gsharePredictorTableEntries;i++)
	{
		predictorTableGshare[i].bit = 2;
	}

	for(i=0;i<chooserTableEntries;i++)
	{
		chooserPredictorTable[i].bit = 1;
	}

		
	while( (fgets(readAddress, 100, traceFile) ) != NULL )
	{
		hexAddress = strtok(readAddress, " ");
		branchActuallyTaken = strtok(NULL, "\n");
		addressInInt = strtoll(hexAddress, NULL, 16);
		
		noOfPredictions++;
		gshareTrueFalse = 0;
		bimodalTrueFalse = 0;

		
		if( btbBuffer->size != 0)
		{
			
			getVal = readFromBTB(btbBuffer, addressInInt);
		}



		if( (btbBuffer->size == 0) || (getVal == 1) )
		{
			btbBuffer->noOfPredictionsFromBTB++;

			//get chooser location
			extractIndexPredictionTable(addressInInt, kBitsChooser, &chooserIndexLocation);


			//Bimodal Entry checking
			extractIndexPredictionTable(addressInInt, m2Bimodal, &indexLocationBimodal);
			

			//Gshare Entry checking	
			extractIndexPredictionTable(addressInInt, m1GShare, &indexLocationGshare);
				
			constructN_BHR_Bits = 0;
			constructN_Bits_From_IndexLoc = 0;
			
			//get nBitsGlobalBHR bits from BHR
			for( i=0; i<nBitsGlobalBHR; i++)
			{
				constructN_BHR_Bits = ( 1 | constructN_BHR_Bits<<1 );
			}
			globalBHR = ( globalBHR & constructN_BHR_Bits );
				
			

			//get the uppermost nBitsGlobalBHR bits from indexLocation/gshare
			constructN_Bits_From_IndexLoc = (indexLocationGshare>>(m1GShare-nBitsGlobalBHR));
			

			//XOR globalBHR and constructN_Bits_From_IndexLoc
			constructN_Bits_From_IndexLoc = (globalBHR^constructN_Bits_From_IndexLoc);
			

			construct_Low_Order_Index_Bits = 0;
			//after XOR extract lower m bits from indexLocation and paste it with higher order n bits of XOR results
			for( i=0; i<(m1GShare-nBitsGlobalBHR); i++)
			{
				construct_Low_Order_Index_Bits = ( 1 | construct_Low_Order_Index_Bits<<1 );
			}
			construct_Low_Order_Index_Bits = (indexLocationGshare & construct_Low_Order_Index_Bits);
			

			//Shift N bits got from XOR'ed to left by (n-m) bits, so that lower m bits can be pasted with it
			constructN_Bits_From_IndexLoc = ( constructN_Bits_From_IndexLoc<<(m1GShare-nBitsGlobalBHR) );
			
			
			//Final Index Location
			indexLocationGshare = (constructN_Bits_From_IndexLoc|construct_Low_Order_Index_Bits);
			

			if( strcmp(branchActuallyTaken, "t") == 0 )
			{
				if(predictorTableGshare[indexLocationGshare].bit >= 2)
					gshareTrueFalse = 1;
				else
					gshareTrueFalse = 0;
				
				if(predictorTableBimodal[indexLocationBimodal].bit >= 2)
					bimodalTrueFalse = 1;
				else
					bimodalTrueFalse = 0;
			}
			else if( strcmp(branchActuallyTaken, "n") == 0 )
			{
				if(predictorTableGshare[indexLocationGshare].bit <=1 )
					gshareTrueFalse = 1;
				else
					gshareTrueFalse = 0;
				
				if(predictorTableBimodal[indexLocationBimodal].bit <=1 )
					bimodalTrueFalse = 1;
				else
					bimodalTrueFalse = 0;
			}





			if(chooserPredictorTable[chooserIndexLocation].bit >=2 )
			{
				//update Gshare predictor values
				
				if( predictorTableGshare[indexLocationGshare].bit >= 2 )
				{
					//branch predicted taken
					if( (strcmp(branchActuallyTaken, "t") == 0) && predictorTableGshare[indexLocationGshare].bit !=3 )
					{
						predictorTableGshare[indexLocationGshare].bit = predictorTableGshare[indexLocationGshare].bit + 1;
					}
					else if( (strcmp(branchActuallyTaken, "n") == 0) && predictorTableGshare[indexLocationGshare].bit != 0 )
					{
						predictorTableGshare[indexLocationGshare].bit = predictorTableGshare[indexLocationGshare].bit - 1;
						noOfMisPredictions++;
					}
				}
				else
				{
					//branch predicted not-taken
					
					if( (strcmp(branchActuallyTaken, "t") == 0) && predictorTableGshare[indexLocationGshare].bit !=3 )
					{
						predictorTableGshare[indexLocationGshare].bit = predictorTableGshare[indexLocationGshare].bit + 1;
						noOfMisPredictions++;
					}
					else if( (strcmp(branchActuallyTaken, "n") == 0) && predictorTableGshare[indexLocationGshare].bit != 0 )
					{
						predictorTableGshare[indexLocationGshare].bit = predictorTableGshare[indexLocationGshare].bit - 1;
					}
				}
			}
			else
			{
				//update BIMODAL predictor values
				if( predictorTableBimodal[indexLocationBimodal].bit >= 2 )
				{
					//branch predicted taken
					if( (strcmp(branchActuallyTaken, "t") == 0) && predictorTableBimodal[indexLocationBimodal].bit !=3 )
					{
						predictorTableBimodal[indexLocationBimodal].bit = predictorTableBimodal[indexLocationBimodal].bit + 1;
					}
					else if( (strcmp(branchActuallyTaken, "n") == 0) && predictorTableBimodal[indexLocationBimodal].bit != 0 )
					{
						predictorTableBimodal[indexLocationBimodal].bit = predictorTableBimodal[indexLocationBimodal].bit - 1;
						noOfMisPredictions++;
					}
				}
				else
				{
					//branch predicted not-taken
					
					if( (strcmp(branchActuallyTaken, "t") == 0) && predictorTableBimodal[indexLocationBimodal].bit !=3 )
					{
						predictorTableBimodal[indexLocationBimodal].bit = predictorTableBimodal[indexLocationBimodal].bit + 1;
						noOfMisPredictions++;
					}
					else if( (strcmp(branchActuallyTaken, "n") == 0) && predictorTableBimodal[indexLocationBimodal].bit != 0 )
					{
						predictorTableBimodal[indexLocationBimodal].bit = predictorTableBimodal[indexLocationBimodal].bit - 1;
					}
				}
			}




			//update the BHR by shifting right
			globalBHR = globalBHR>>1;
			
				
			constructN_BHR_Bits = 0;
			constructN_Bits_From_IndexLoc = 1;
			for( i=0; i<(nBitsGlobalBHR-1); i++)
			{
				constructN_BHR_Bits = ( 1 | constructN_BHR_Bits<<1 );
				constructN_Bits_From_IndexLoc = ( 0 | constructN_Bits_From_IndexLoc<<1 );
			}


			//At this time I have (n-1) bits BHR with nth position as 0.
			globalBHR = ( globalBHR & constructN_BHR_Bits );
				


			if( (strcmp(branchActuallyTaken, "t") == 0) )
			{
				globalBHR = ( constructN_Bits_From_IndexLoc| globalBHR );
			}




			if( gshareTrueFalse == 1 && bimodalTrueFalse == 0)
			{
				if( chooserPredictorTable[chooserIndexLocation].bit !=3 )
					chooserPredictorTable[chooserIndexLocation].bit = chooserPredictorTable[chooserIndexLocation].bit + 1;
			}


			if( gshareTrueFalse == 0 && bimodalTrueFalse == 1)
			{
				if( chooserPredictorTable[chooserIndexLocation].bit !=0 )
					chooserPredictorTable[chooserIndexLocation].bit = chooserPredictorTable[chooserIndexLocation].bit - 1;
			}	
		}
		

		if( (btbBuffer->size != 0) && (getVal == 0) )
		{
			//branch is not at all or predicted not-taken
			//BTB miss
			//take account of those branches taken
			if( (strcmp(branchActuallyTaken, "t") == 0) )
				btbBuffer->BTBMissButBranchTaken++;

		}
	}


	//BTB contents
	int j= 0;
	if( btbBuffer->size!=0)
	{
		printf("size of BTB:	 %d\n", btbBuffer->size);
		printf("number of branches:		%d\n", btbBuffer->branchCounter);
		printf("number of predictions from branch predictor:	%d\n", btbBuffer->noOfPredictionsFromBTB);
		printf("number of mispredictions from branch predictor: %d\n", noOfMisPredictions);
		printf("number of branches miss in BTB and taken:	 %d\n", btbBuffer->BTBMissButBranchTaken);
		printf("total mispredictions:	%d\n", (noOfMisPredictions+btbBuffer->BTBMissButBranchTaken));

		double missrate = ((double)(noOfMisPredictions+btbBuffer->BTBMissButBranchTaken) / btbBuffer->branchCounter)*100;
		printf("misprediction rate:	%0.2f%%\n", missrate);

		printf("\nFINAL BTB CONTENTS\n");
		for( i=0; i<(int)btbBuffer->numOfSets; i++)
		{
			//sort L1 cache based on LRU counter
			printf("set %-4d:	", i);
			for( j=0; j<(int)btbBuffer->assoc; j++)
			{
				printf("%-10x",btbBuffer->tagArray[i + (j*btbBuffer->numOfSets)]);
			}
			printf("\n");
		}

		printf("\nFINAL CHOOSER CONTENTS\n");
		for(i=0;i<chooserTableEntries;i++)
		{
			printf("%d	%d\n", i, chooserPredictorTable[i].bit );
		}


		printf("\nFINAL GSHARE CONTENTS\n");
		for(i=0;i<gsharePredictorTableEntries;i++)
		{
			printf("%d	%d\n", i, predictorTableGshare[i].bit );
		}


		printf("\nFINAL BIMODAL CONTENTS\n");
		for(i=0;i<bimodalPredictorTableEntries;i++)
		{
			printf("%d	%d\n", i, predictorTableBimodal[i].bit );
		}
	}
	else
	{
		
		printf("number of predictions: %d\n", noOfPredictions);
		printf("number of mispredictions: %d\n", noOfMisPredictions);
		double missrate = ((double)noOfMisPredictions / noOfPredictions)*100;
		printf("misprediction rate: %0.2f%%\n", missrate);

		printf("FINAL CHOOSER CONTENTS\n");
		for(i=0;i<chooserTableEntries;i++)
		{
			printf("%d	%d\n", i, chooserPredictorTable[i].bit );
		}


		printf("FINAL GSHARE CONTENTS\n");
		for(i=0;i<gsharePredictorTableEntries;i++)
		{
			printf("%d	%d\n", i, predictorTableGshare[i].bit );
		}


		printf("FINAL BIMODAL CONTENTS\n");
		for(i=0;i<bimodalPredictorTableEntries;i++)
		{
			printf("%d	%d\n", i, predictorTableBimodal[i].bit );
		}
	
	}
	
	
	return(1);
}