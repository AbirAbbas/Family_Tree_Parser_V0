#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <ctype.h>
#include "assert.h"
#include <unistd.h>

#include "LinkedListAPI.h"
#include "GEDCOMparser.h"

typedef struct {
	char ** tokenizedString;
	int size;
} stringInformation;

typedef struct {
	char * address;
	List * listPointer;
	Individual ** spousePointer;
	Submitter ** submitterPointer;
	void * initializedPointer;
	int type;
	int count;
} addressInformation;

GEDCOMerror createError (ErrorCode type, int line);

bool fileExist (const char * fileName);

void removeEnter (char * text);

stringInformation stringTokenizer(char * input);

void initObject (GEDCOMobject ** obj);

int returnSize (char ** array);

bool checkTags (char * tagContainer[], char * tag);

Individual * createIndividual ();

Family * createFamily ();

bool compareFindPerson(const void* first,const void* second);

//all level 0's
static char * recordTags[] = { "HEAD", "SUBM", "INDI", "FAM", "TRLR", NULL };
//all level 1's
static char * headerTags[] = { "SOUR", "GEDC", "SUBM", NULL };

/*

static char * individualTags[] = {  };

static char * familyTags[] = {  };

static char * submissionTags[] = {  };
* */

	
static char * eventTags[] = { "BIRT", "CHR", "ADOP", "DEAT", "BURI", "CREM",
	"BAPM", "BARM", "BASM", "BLES", "CHRA", "CONF", "FCOM", "ORDN", "NATU", "EMIG",
	"EMIG", "IMMI", "CENS", "PROB", "WILL", "GRAD", "RETI", "EVEN"  };
	
static char * feventTags[] = { "ANUL", "CENS", "DIV", "DIVF", "ENGA", "MARB", "MARC", "MARR", "MARL", "MARS" };


typedef enum rCode {HEADER, SUBMITTER, FAMILY, INDIVIDUAL, NONE} recordSet;

typedef enum sCode {GEDC, SUBM, NAME, INDI, EVEN, NOSUB} subSet;

GEDCOMerror parseDistributor(stringInformation splitString, GEDCOMobject * obj, int counter, recordSet * currentRecord, subSet * subRecord, List * addressList, List * recieveList, int * checkHeader);

char * combineString (char ** string, int initial, int final);

GEDCOMerror regularParser (stringInformation splitString, GEDCOMobject * obj, int counter, recordSet * currentRecord, subSet * subRecord, bool isToPointer, List * addressList, List * recieveAddress, int * checkHeader);

GEDCOMerror pointerParser(stringInformation splitString, GEDCOMobject * obj, int counter, recordSet * currentRecord, subSet * subRecord, List * addressList, int * checkHeader);

bool checkLevel(char * fileName, int level, int line);

void deleteAddress(void* toBeDeleted);

int compareAddress(const void* first,const void* second);

char* printAddress(void* toBePrinted);

addressInformation * createAddress (List * listPointer, Submitter ** submitterPointer, Individual ** spousePointer, char * address, void * initializedPointer, int count);

void freeStringArray (stringInformation splitString);

bool findInLinker(List addressList, void * data);

void freeLists(List addressList, List recieveList);

bool checkHeaderInit(Header * h, int * submitterFlag);

bool isAlpha (char * string);

void recursiveDescendant(List * descendants, Individual * i);

bool checkIfSpouse (Family * f, Individual * i);

bool checkIfExists(List * l, Individual * i);

Individual * createCopy(Individual * input);


/** Function to create a GEDCOM object based on the contents of an GEDCOM file.
 *@pre File name cannot be an empty string or NULL.  File name must have the .ged extension.
 File represented by this name must exist and must be readable.
 *@post Either:
 A valid GEDCOM has been created, its address was stored in the variable obj, and OK was returned
 or
 An error occurred, the GEDCOM was not created, all temporary memory was freed, obj was set to NULL, and the
 appropriate error code was returned
 *@return the error code indicating success or the error encountered when parsing the GEDCOM
 *@param fileName - a string containing the name of the GEDCOM file
 *@param a double pointer to a GEDCOMobject struct that needs to be allocated
 **/
GEDCOMerror createGEDCOM(char* fileName, GEDCOMobject** obj) {
	
	FILE * inFile = NULL;
	int level = 0;
	int currentLevel = 0;
	int counter = 1;
	int * checkHeader = calloc(1, sizeof(int));
	*checkHeader = 0;
	long int currentLocation = 0;
	char input[5000];
	char tempConc[5000];
	char saveTRLR[5000];
	char trailerCharacter[5000];
	bool filePass = false;
	recordSet * currentRecord = calloc(1, sizeof(recordSet));
	subSet * subRecord = calloc(1, sizeof(subSet));
	*currentRecord = NONE;
	*subRecord = NOSUB;
	List addressList = initializeList(printAddress, deleteAddress, compareAddress);
	List recieveList = initializeList(printAddress, deleteAddress, compareAddress);
	
	//fileValidator : checks if file exists, if fileName is NULL, and if fileName ends with .ged
	if (fileName != NULL) {
			
		if ((strlen(fileName) - 3) > 0) {
			if (fileName[strlen(fileName) - 4] == '.' && fileName[strlen(fileName) - 3] == 'g' && fileName[strlen(fileName) - 2] == 'e' && fileName[strlen(fileName) - 1] == 'd') {
				if (fileExist(fileName)) {
					filePass = true;
				}
			}
		}
	}
	//Returns error code if failed!
	if (filePass == false) {
		obj = NULL;
		free(currentRecord);
		free(subRecord);
		return createError((ErrorCode)INV_FILE, -1);
	}
	
	initObject(&(*obj));
		
	//Opens the file if no error was generated
	inFile = fopen(fileName, "r");

	//Reads in a sentence from the provided GEDCOM file line by line
	while (fgets(input, 1024, inFile) != NULL) {
		
		sscanf(input, "%d", &level);
		
		if ((level - currentLevel) > 1) {
			if (*currentRecord == HEADER || *currentRecord == NONE) {
				deleteGEDCOM(*obj);
				free(checkHeader);
				freeLists(addressList, recieveList);
				fclose(inFile);
				free(currentRecord);
				free(subRecord);
				return createError(INV_HEADER, counter);
			}
			else {
				deleteGEDCOM(*obj);
				free(checkHeader);
				freeLists(addressList, recieveList);
				fclose(inFile);
				free(currentRecord);
				free(subRecord);
				return createError(INV_RECORD, counter);
			}
		}
		
		//printf("%d\n", checkLevel(fileName, level, counter));
		
		//Removes enter from fgets
		strcpy(saveTRLR, input);
		removeEnter(input);

		//Checks the length, if greated than 255 returns error code, counter is the line number!
		if ((int)strlen(input) > 255) {
			deleteGEDCOM(*obj);
			freeLists(addressList, recieveList);
			fclose(inFile);
			free(currentRecord);
			free(subRecord);
			free(checkHeader);
			return createError((ErrorCode)INV_RECORD, counter);
		}
		
		//tokenizes the string and returns a 2D array as well as the size in a struct Union
		strcpy(trailerCharacter, input);
		
		currentLocation = ftell(inFile);
		
		while (fgets(tempConc, 256, inFile) != NULL && (strstr(tempConc, "CONT") != NULL || strstr(tempConc, "CONC") != NULL)) {
			if (strstr(tempConc, "CONT") != NULL) {
				stringInformation splitConc = stringTokenizer(tempConc);
				if (strcmp(splitConc.tokenizedString[1], "CONT") == 0) {
					strcpy(input, saveTRLR);
					char * tempLeakfix = combineString(splitConc.tokenizedString, 3, splitConc.size);
					strcat(input, tempLeakfix);
					free(tempLeakfix);
					strcpy(saveTRLR, input);
					currentLocation = ftell(inFile);
				}
				freeStringArray(splitConc);
			}
			else if (strstr(tempConc, "CONC") != NULL) {
				stringInformation splitConc = stringTokenizer(tempConc);
				if (strcmp(splitConc.tokenizedString[1], "CONC") == 0) {
					strcat(input, " ");
					char * tempLeakfix = combineString(splitConc.tokenizedString, 3, splitConc.size);
					strcat(input, tempLeakfix);
					free(tempLeakfix);
					removeEnter(input);
					currentLocation = ftell(inFile);
				}
				//printf("\n\n%s\n", input);
				freeStringArray(splitConc);
			}
			else {	
				strcpy(tempConc, "");
				break;			
			}

		}
		
		fseek(inFile, currentLocation, SEEK_SET);
		
		stringInformation splitString = stringTokenizer(input);
		

		GEDCOMerror checkParser = parseDistributor(splitString, *obj, counter, currentRecord, subRecord, &addressList, &recieveList, checkHeader);
		
		if (checkParser.type != OK) {
			deleteGEDCOM(*obj);
			freeLists(addressList, recieveList);
			freeStringArray(splitString);
			fclose(inFile);
			free(currentRecord);
			free(subRecord);
			free(checkHeader);
			return checkParser;
		}
		
		
		freeStringArray(splitString);
		
		splitString.tokenizedString = NULL;
		//Line counter incremented by 1 every time a line is read!
		counter++;
		currentLevel = level;
	}
	
	fseek(inFile, 0, SEEK_SET);
	
	while (fgets(input, 1024, inFile) != NULL) { 
		
	}
	
	
	
	stringInformation splitString = stringTokenizer(trailerCharacter);
	
	if (splitString.size != 2) {
		deleteGEDCOM(*obj);
		freeLists(addressList, recieveList);
		freeStringArray(splitString);
		fclose(inFile);
		free(currentRecord);
		free(subRecord);
		free(checkHeader);
		return createError(INV_GEDCOM, counter);
	}
	else if (strcmp(splitString.tokenizedString[0], "0") != 0 || strcmp(splitString.tokenizedString[1], "TRLR") != 0) {
		deleteGEDCOM(*obj);
		freeLists(addressList, recieveList);
		freeStringArray(splitString);
		fclose(inFile);
		free(currentRecord);
		free(subRecord);
		free(checkHeader);
		return createError(INV_GEDCOM, counter);
	}
	
	freeStringArray(splitString);
	
	free(currentRecord);
	free(subRecord);
	fclose(inFile);
		
	Node * n = recieveList.head;
		
	while (n!=NULL) {
		
		if (!findInLinker(addressList, n->data)) {
			//Invalid address
			deleteGEDCOM(*obj);
			freeLists(addressList, recieveList);
			freeStringArray(splitString);
			fclose(inFile);
			free(currentRecord);
			free(subRecord);
			free(checkHeader); 
			return createError(INV_GEDCOM, -1);
		}

		n = n->next;
	}
		
	freeLists(addressList, recieveList);
	
	free(checkHeader);

	return createError((ErrorCode)OK, -1);
}

void freeStringArray (stringInformation splitString) {
	//Memory is free'd dynamically based on size of string array
	for (int i = 0; i < splitString.size; i++) {
		free(splitString.tokenizedString[i]);
	}
	//pointer is free'd and set to NULL for the next iteration to avoid memory errors
	free(splitString.tokenizedString);
	splitString.tokenizedString = NULL;
}

/** Function to create a string representation of a GEDCOMobject.
 *@pre GEDCOMobject object exists, is not null, and is valid
 *@post GEDCOMobject has not been modified in any way, and a string representing the GEDCOM contents has been created
 *@return a string contaning a humanly readable representation of a GEDCOMobject
 *@param obj - a pointer to a GEDCOMobject struct
 **/
char* printGEDCOM(const GEDCOMobject* obj) {
	char * temp = calloc(1024, sizeof(char));
	char * tempHolder = NULL;
	
	strcat(temp, "Header Information : \n");
	
	if (obj->header != NULL) {
		Header * h = obj->header;
		if (h->source != NULL) {
			strcat(temp, h->source);
			strcat(temp, "\n");
		}
		char * num = malloc(sizeof(char) * 32);
		if (h->gedcVersion >= 0) {
			strcat(temp, "GEDC Version : ");
			sprintf(num, "%f", h->gedcVersion);
			if (num != NULL) {
				strcat(temp, num);
				free(num);
			}
			strcat(temp, "\n");
		}
	}
	
	if (obj->submitter != NULL) {
		Submitter * s = obj->submitter;
		
		if (s->submitterName != NULL) {
			strcat(temp, "Submitter Name : ");
			strcat(temp, s->submitterName);
			strcat(temp, "\n");
		}
		
		if (s->address != NULL) {
			strcat(temp, "Submitter Address : ");
			strcat(temp, s->address);
			strcat(temp, "\n");
		}
		
		tempHolder = toString(s->otherFields);
		
		if (tempHolder != NULL) {
			temp = realloc(temp, (strlen(temp) + strlen(tempHolder) + 50) * sizeof(char));
			strcat(temp, "Header, OtherFields : \n");
			strcat(temp, tempHolder);
			free(tempHolder);
			strcat(temp, "\n");
		}
	}
	
	temp = realloc(temp, (strlen(temp)+ 1024) * sizeof(char));
	
	strcat(temp, "Header information completed\n");
	
	char * allOtherFields = toString(obj->families);
	
	if (allOtherFields != NULL) {
		temp = realloc(temp, (strlen(temp) + strlen(allOtherFields) + 1024) * sizeof(char));
		strcat(temp, allOtherFields);
		free(allOtherFields);
	}
	
	strcat(temp, "All information has been completely printed!\n");
	
	
	
		
	return temp;
}

/** Function to delete all GEDCOM object content and free all the memory.
 *@pre GEDCOM object exists, is not null, and has not been freed
 *@post GEDCOM object had been freed
 *@return none
 *@param obj - a pointer to a GEDCOMobject struct
 **/
void deleteGEDCOM(GEDCOMobject * obj) {
	
	if (obj == NULL) {
		return;
	}
	
	Header * h = obj->header;
	Submitter * sub = obj->submitter;
	
	//Header started
	
	if (h != NULL) {
		clearList(&(h->otherFields));
		free(h);
	//Header deleted
	}
		
	//printf("TESSST\n");
	//Families started
	clearList(&(obj->families));
	//FamiliesDeleted
	
	//Individuals started
	clearList(&(obj->individuals));
	//Invidiaul deleted
	
	//Submitter->otherFields started
	if (sub!= NULL) {
		clearList(&(sub->otherFields));
		free(sub);
	}
	//Submitter->otherFields deleted
}

/** Function to "convert" the GEDCOMerror into a humanly redabale string.
 *@return a string contaning a humanly readable representation of the error code
 *@param err - an error struct
 **/
char* printError(GEDCOMerror err) {
	char * error = calloc(1024, sizeof(char));
	if (err.type == OK) {
		strcpy(error, "No errors reported\n");
	}
	else if (err.type == INV_FILE) {
		strcpy(error, "Error while opening file of format .ged!\n");
	}
	else if (err.type == INV_GEDCOM) {
		strcpy(error, "Invalid Gedcom object, Missing header or TRLR character!\n");
	}
	else if (err.type == INV_HEADER) {
		strcpy(error, "Invalid Header Object, at line ");
		char * num = malloc(sizeof(char) * 32);
		sprintf(num, "%d", err.line);
		if (num != NULL) {
			strcat(error, num);
			free(num);
		}
		strcat(error, "\n");
	}
	else if (err.type == INV_RECORD) {
		strcpy(error, "Invalid Record Object, at line ");
		char * num = malloc(sizeof(char) * 32);
		sprintf(num, "%d", err.line);
		if (num != NULL) {
			strcat(error, num);
			free(num);
		}
		strcat(error, "\n");
	}
	else if (err.type == OTHER_ERROR) {
		strcpy(error, "System out of memory!\n");
	}
	
	return error;
}

/** Function that searches for an individual in the list using a comparator function.
 * If an individual is found, a pointer to the Individual record
 * Returns NULL if the individual is not found.
 *@pre GEDCOM object exists,is not NULL, and is valid.  Comparator function has been provided.
 *@post GEDCOM object remains unchanged.
 *@return The Individual record associated with the person that matches the search criteria.  If the Individual record is not found, return NULL.
 *If multiple records match the search criteria, return the first one.
 *@param familyRecord - a pointer to a GEDCOMobject struct
 *@param compare - a pointer to comparator fuction for customizing the search
 *@param person - a pointer to search data, which contains seach criteria
 *Note: while the arguments of compare() and person are all void, it is assumed that records they point to are
 *      all of the same type - just like arguments to the compare() function in the List struct
 **/
Individual* findPerson(const GEDCOMobject* familyRecord, bool (*compare)(const void* first, const void* second), const void* person) {
	
	Node * n = familyRecord->individuals.head;
	
	while (n!= NULL) {
		if (compare(person, n->data) == true) { // FOUND
			Individual * data = n->data;
			
			return data;
		}
		n = n->next;
	}
	
	return NULL;
}

/** Function to return a list of all descendants of an individual in a GEDCOM
 *@pre GEDCOM object exists, is not null, and is valid
 *@post GEDCOM object has not been modified in any way, and a list of descendants has been created
 *@return a list of descendants.  The list may be empty.  All list members must be of type Individual, and can appear in any order.
 *All list members must be COPIES of the Individual records in the GEDCOM file.  If the returned list is freed, the original GEDCOM
 *must remain unaffected.
 *@param familyRecord - a pointer to a GEDCOMobject struct
 *@param person - the Individual record whose descendants we want
 **/
List getDescendants(const GEDCOMobject* familyRecord, const Individual* person) {
	
	List descendants = initializeList(printIndividual, deleteIndividual, compareIndividuals);
	
	Individual * i = findPerson(familyRecord, compareFindPerson, person);
	
	if (i == NULL) {
		return descendants;
	}
	
	recursiveDescendant(&descendants, i); 
	
	//Individual * i = findPerson(familyRecord, compareFindPerson, person);

	return descendants;
}

void recursiveDescendant(List * descendants, Individual * i) {
	if (i == NULL) {
		return;
	}
	if (i->families.length == 0) {
		return;
	}
	
	Node * n = i->families.head;
	
	while (n != NULL) {
		if (checkIfSpouse(n->data, i)) {
			Node * e = ((Family*)(n->data))->children.head;
			while (e!= NULL) {
				if (!compareFindPerson(i, e->data)) {
					recursiveDescendant(descendants, e->data);
					if (!checkIfExists(descendants, e->data)) {
						insertBack(descendants, createCopy(e->data));
					}
				}
				e=e->next;
			}
		}
		n = n->next;
	}
}

bool checkIfSpouse (Family * f, Individual * i) {
	if (f->husband == NULL) {
		return false;
	}
	else if (f->wife == NULL) {
		return false;
	}
	
	else if (i==NULL) {
		return false;
	}
	
	else if (compareFindPerson(f->husband, i)) {
		return true;
	}
	
	else if (compareFindPerson(f->wife, i)) {
		return true;
	}
	else {
		return false;
	}
}

bool checkIfExists(List * l, Individual * i) {
	Node * n = l->head;
	
	while (n!=NULL) {
		if (compareFindPerson(n->data, i)) {
			return true;
		}
		n = n->next;
	}
	return false;	
}

// ******************* HELPER FUNCTIONS ***********************
//EVENTS
void deleteEvent(void* toBeDeleted) {
	
	if (toBeDeleted == NULL) {
		return;
	}
	
	Event * e = ((Event*)toBeDeleted);
	
	if (e->date != NULL) free(e->date);
	if (e->place != NULL) free(e->place);
	
	clearList(&(e->otherFields));
	
	free(e);
}

int compareEvents(const void* first,const void* second) {
	
	if (first == NULL || second == NULL) {
		return -1;
	}
	
	char * one;
	char * two;
	
	one = ((Event*)first)->type;
	two = ((Event*)first)->type;
	
	return strcmp(one, two);

}

char* printEvent(void* toBePrinted) {
	if (toBePrinted == NULL) {
		char * e = calloc(1, sizeof(char) * 2);
		strcpy(e, "\n");
		return e;
	}
	
	Event * e = toBePrinted;
	
	char * printEvent = calloc(2, sizeof(char));
	char * tempHolder = calloc(2048, sizeof(char));
	strcpy(printEvent, "");
	strcpy(tempHolder, "");
	strcat(tempHolder, "--------Event-------\n");
	if (e->type != NULL) { 
		strcat(tempHolder, "Type : ");
		strcat(tempHolder, e->type);
		strcat(tempHolder, "\n");
	}
	
	if (e->date != NULL) { 
		strcat(tempHolder, "Date : ");
		strcat(tempHolder, e->date);
		strcat(tempHolder, "\n");
	}
	
	if (e->place != NULL) { 
		strcat(tempHolder, "Place : ");
		strcat(tempHolder, e->place);
		strcat(tempHolder, "\n");
	}
	
	if (tempHolder != NULL) {
		printEvent = (char*)realloc(printEvent, (strlen(printEvent) + strlen(tempHolder) + 50) * sizeof(char));
		strcpy(printEvent, tempHolder);
		free(tempHolder);
	}
	
	tempHolder = toString(e->otherFields);
	
	if (tempHolder != NULL) {
		printEvent = (char*)realloc(printEvent, (strlen(printEvent) + strlen(tempHolder) + 150) * sizeof(char));	
		strcat(printEvent, tempHolder);
		free(tempHolder);
	}
	
	strcat(printEvent, "-----END OF EVENT----\n");
	
	return printEvent;
}

//INDIVIDUAL
void deleteIndividual(void* toBeDeleted) {
	
	if (toBeDeleted == NULL) {
		return;
	}
	
	Individual * i = (Individual*)toBeDeleted;
	
	if (i->givenName!=NULL) {
		free(i->givenName);
	}
	
	if (i->surname != NULL) {
		free(i->surname);
	}
	
	clearList(&(i->events));
	clearList(&(i->otherFields));
	
	Node * n = i->families.head;
	Node * delete;
	
	while (n != NULL) {
		delete = n;
		n = n->next;
		free(delete);
	}
	
	free(i);
	
}

int compareIndividuals(const void* first,const void* second) {
	
	char temp[1024];
	
	strcpy(temp, ((Individual*)first)->givenName);
	strcat(temp, ", ");
	strcat(temp, ((Individual*)first)->surname);
	
	char secondTemp[1024];
	
	strcpy(secondTemp, ((Individual*)second)->givenName);
	strcat(secondTemp, ", ");
	strcat(secondTemp, ((Individual*)second)->surname);
	
	
	return strcmp(temp, secondTemp);
}


char* printIndividual(void* toBePrinted) {
	
	if (toBePrinted == NULL) {
		char * e = calloc(1,sizeof(char) * 2);
		strcpy(e, "\n");
		return e;
	}
	
	Individual * e = toBePrinted;
	
	char * printField = calloc(1,sizeof(char));
	char * tempHolder = calloc(1,sizeof(char) * 2048);
	
	strcpy(tempHolder, "	@@@@ - Individual - @@@@\n");
	
	if (e->givenName != NULL) { 
		strcat(tempHolder, "	Given Name : ");
		strcat(tempHolder, e->givenName);
		strcat(tempHolder, "\n");
	}
	
	if (e->surname != NULL) { 
		strcat(tempHolder, "	Surname : ");
		strcat(tempHolder, e->surname);
		strcat(tempHolder, "\n");
	}

	
	if (tempHolder != NULL) {
		printField = (char*)realloc(printField, (strlen(printField) + strlen(tempHolder) + 50) * sizeof(char));	
		strcpy(printField, tempHolder);
		free(tempHolder);
	}
	
	tempHolder = toString(e->events);
	
	if (tempHolder != NULL) {
		printField = (char*)realloc(printField, (strlen(printField) + strlen(tempHolder) + 150) * sizeof(char));	
		strcat(printField, tempHolder);
		free(tempHolder);
	}
	
	tempHolder = toString(e->otherFields);
	
	if (tempHolder != NULL) {
		printField = (char*)realloc(printField, (strlen(printField) + strlen(tempHolder) + 150) * sizeof(char));	
		strcat(printField, tempHolder);
		free(tempHolder);
	}
	
	strcat(printField, "	@@@@@@@@ - END - @@@@@@@@\n");

	return printField;
}



//FAMILY
void deleteFamily(void* toBeDeleted) {
	
	if (toBeDeleted == NULL) {
		return;
	}
	
	Family * f = (Family*)toBeDeleted;
	
	clearList(&(f->otherFields));
	
	clearList(&(f->events));
	
	Node * n = f->children.head;
	Node * delete;
	
	while (n != NULL) {
		delete = n;
		n = n->next;
		free(delete);
	}
	
	f->wife = NULL;
	f->husband = NULL;
	
	free(f);
}

int compareFamilies(const void* first,const void* second) {
	
	int one = ((List*)first)->length;
	int two = ((List*)second)->length;
	
	if (one < two) {
		return -1;
	}
	else if (one > two) {
		return 1;
	}
	else {
		return 0;
	}
}


char* printFamily(void* toBePrinted) {
	if (toBePrinted == NULL) {
		char * e = calloc(1,sizeof(char) * 2);
		strcpy(e, "\n");
		return e;
	}
	
	Family * e = toBePrinted;
	
	char * printField = calloc(1,sizeof(char));
	char * tempHolder = calloc(1,sizeof(char) * 2048);
	
	
	strcpy(tempHolder, "");
	strcat(tempHolder, "***********************************************FAMILY************************************************\n");
	
	if (e->husband != NULL) { 
		strcat(tempHolder, "Husband : \n");
		char * husband = printIndividual(e->husband);
		if (husband != NULL) {
			strcat(tempHolder, husband);
			free(husband);
		}
		strcat(tempHolder, "\n");
	}
	
	if (e->wife != NULL) { 
		strcat(tempHolder, "Wife : \n");
		char * wife = printIndividual(e->wife);
		if (wife != NULL) {
			strcat(tempHolder, wife);
			free(wife);
		}
		strcat(tempHolder, "\n");
	}
	
	if (tempHolder != NULL) {
		printField = (char*)realloc(printField, (strlen(tempHolder) + 50) * sizeof(char));	
		strcpy(printField, tempHolder);
		free(tempHolder);
	}
	
	tempHolder = toString(e->events);
	
	if (tempHolder != NULL) {
		printField = (char*)realloc(printField, (strlen(printField) + strlen(tempHolder) + 150	) * sizeof(char));
		strcat(printField, "Family Events : \n");
		strcat(printField, tempHolder);
		free(tempHolder);
		strcat(printField, "\n");
	}
	
	tempHolder = toString(e->otherFields);
	
	if (tempHolder != NULL) {
		printField = (char*)realloc(printField, (strlen(printField) + strlen(tempHolder) + 150	) * sizeof(char));
		strcat(printField, "Other fields : \n");
		strcat(printField, tempHolder);
		strcat(printField, "\n");
		free(tempHolder);
	}
	
	strcat(printField, "****************************************END OF FAMILY********************************************\n");
	
		
	return printField;
}


//FIELD
void deleteField(void* toBeDeleted) {
	
	Field * f = (Field*)toBeDeleted;
	
	if (f->tag != NULL) {
		free(f->tag);
	}
	if(f->value != NULL) {
		free(f->value);
	}
	
	free(f);
	
}

int compareFields(const void* first,const void* second) {
	
	if (first == NULL || second == NULL) {
		return -1;
	}

	char * one = ((Field*)first)->tag;
	char * two = ((Field*)second)->tag;
	
	return strcmp(one, two);
}

char* printField(void* toBePrinted) {
	if (toBePrinted == NULL) {
		return NULL;
	}
	
	Field * e = toBePrinted;
	
	char * printField = calloc(1, sizeof(char) * 2);
	char * tempHolder = calloc(2048, sizeof(char));
	
	strcpy(printField, "");
	strcpy(tempHolder, "");
	strcat(tempHolder, "Field { \n");
	
	if (e->tag != NULL) { 
		strcat(tempHolder, "   Tag : ");
		strcat(tempHolder, e->tag);
		strcat(tempHolder, "\n");
	}
	
	if (e->value != NULL) { 
		strcat(tempHolder, "   Value : ");
		strcat(tempHolder, e->value);
		strcat(tempHolder, "\n");
	}
	
	strcat(tempHolder, "} End of Field\n");
	
	if (tempHolder != NULL) {
		printField = (char*)realloc(printField, (strlen(printField) + strlen(tempHolder) + 50) * sizeof(char));
	
		strcpy(printField, tempHolder);
	
		free(tempHolder);
	}
	
	return printField;
}

GEDCOMerror createError (ErrorCode type, int line) {
	GEDCOMerror errorContainer;
	errorContainer.type = type;
	errorContainer.line = line;
	
	return errorContainer;
}

bool fileExist (const char * fileName) {
	
	FILE * f = NULL;
	if ((f = fopen(fileName, "r"))) {
		fclose(f);
		return true;
	}
	
	return false;
}

void removeEnter (char * text) {

	text[strcspn(text, "\r\n")] = 0;
	
}

stringInformation stringTokenizer(char * input) {
	stringInformation tempInfo;
	char **tokenizedString = NULL;
	int countDelim = 0;
	char * temp = NULL;
	
	temp = calloc(strlen(input) + 1, sizeof(char));
	
	strcpy(temp, input);
	
	for (char * counter = strtok(temp, " "); counter != NULL; counter = strtok(NULL, " ")) {
		countDelim++;
	}
	
	tokenizedString = calloc(countDelim + 1, sizeof(char*));
	//tokenizedString = calloc(1,(countDelim + 1));
	strcpy(temp, input);
	
	int size = countDelim;
	
	countDelim = 0;
	
	for (char * counter = strtok(temp, " "); counter != NULL; counter = strtok(NULL, " ")) {
		tokenizedString[countDelim] = calloc(strlen(counter) + 1, sizeof(char));
		//tokenizedString[countDelim] = calloc(1,sizeof(char) * strlen(counter) + 1);
		strcpy(tokenizedString[countDelim], counter);
		countDelim++;
	}
	
	free(temp);
		
	tempInfo.tokenizedString = tokenizedString;
	tempInfo.size = size;
	
	return tempInfo;
}

GEDCOMerror parseDistributor(stringInformation splitString, GEDCOMobject * obj, int counter, recordSet * currentRecord, subSet * subRecord, List * addressList, List * recieveList, int * checkHeader) {
	
	char ** tokenizedString = splitString.tokenizedString;
	bool isAtPointer = false;
	bool isToPointer = false;
	int size = splitString.size;
	int i = 0;
	int level = 0;
	
	
	if (*currentRecord == NONE && *subRecord == NOSUB) {
		if (strcmp(tokenizedString[0], "0") == 0 ) {
			if (strcmp(tokenizedString[1], "HEAD") != 0) {
				return createError(INV_GEDCOM, counter);
			} 
		}
		else if (atoi(tokenizedString[0]) >= 0) {
			return createError(INV_GEDCOM, counter);
		}
	}
	
	//Once all information is gathered through the several tests
	//This function will assign which type parser to assign the data to!
	//Incase a null was slipped into this function by mistake!
	if (size <= 1) {
		return createError(INV_RECORD, counter);
	}
	//check if is number!
	for (i = 0; i < strlen(tokenizedString[0]); i++) {
		if (!isdigit(tokenizedString[0][i])) {
			return createError(INV_RECORD, counter);
		}
	}
	
	//validates level!
	level = strtol(tokenizedString[0], NULL, 10);
	if (level < 0 || level > 99) {
		return createError(INV_RECORD, counter);
	}
	
	//checks whether it is a pointer or not!
	if (tokenizedString[1][0] == '@' && tokenizedString[1][strlen(tokenizedString[1]) - 1] == '@') {
		//As per profs suggestion, create a temporary data structure to hold all pointers!
		//Untill all values are successfully read in and intialized and ready to be connected!
		
		//declare temp structure here!
		isAtPointer = true;		
	}
	else if (size > 2 && tokenizedString[2][0] == '@' && tokenizedString[2][strlen(tokenizedString[2]) - 1] == '@') {
		//printf("%s\n", combineString(tokenizedString, 1, size));
		isToPointer = true;
	}
	else if (tokenizedString[1][0] == '@' && tokenizedString[1][strlen(tokenizedString[1]) - 1] != '@') {
		return createError(INV_RECORD, counter);
	}
	else if (tokenizedString[1][0] != '@' && tokenizedString[1][strlen(tokenizedString[1]) - 1] == '@') {
		return createError(INV_RECORD, counter);
	}
	else if (size > 2 && tokenizedString[2][0] == '@' && tokenizedString[2][strlen(tokenizedString[2]) - 1] != '@') {
		return createError(INV_RECORD, counter);
	}
	else if (size > 2 && tokenizedString[2][0] != '@' && tokenizedString[2][strlen(tokenizedString[2]) - 1] == '@') {
		return createError(INV_RECORD, counter);
	}

	//checks if pointer!
	if (!isAtPointer) {
		if (strlen(tokenizedString[1]) > 4) {
			return createError(INV_RECORD, counter);
		}
		
		if (!isAlpha(tokenizedString[1])) {
			return createError(INV_RECORD, counter);
		}
		
		if (isToPointer) {
			if (strlen(tokenizedString[2]) > 22) {
				return createError(INV_RECORD, counter);
			}
		}
		
		GEDCOMerror n = regularParser(splitString, obj, counter, currentRecord, subRecord, isToPointer, addressList, recieveList, checkHeader);
		
		if (n.type != OK) {
			return n;
		}
	}
	else {
		if (strlen(tokenizedString[2]) > 4) {
			return createError(INV_RECORD, counter);
		}
		
		if (!isAlpha(tokenizedString[2])) {
			return createError(INV_RECORD, counter);
		}
		
		if (strlen(tokenizedString[1]) > 22) {
			return createError(INV_RECORD, counter);
		}
		
		GEDCOMerror e = pointerParser(splitString, obj, counter, currentRecord, subRecord, addressList, checkHeader);
		if (e.type != OK) {
			return e;
		}
	}
	
	//Start from here, required implementations : Tag validator, distributor and initializers
	
		
	return createError((ErrorCode)OK, -1);
}

void initObject (GEDCOMobject ** obj) {	

	(*obj) = calloc(1, sizeof(GEDCOMobject));
	//initialize pointers
	(*obj)->header = calloc(1,sizeof(Header));
	strcpy((*obj)->header->source, "");
	(*obj)->header->gedcVersion = -1;
	
	(*obj)->header->otherFields = initializeList(printField, deleteField, compareFields);
	
	(*obj)->submitter = calloc(256, sizeof(Submitter));
	(*obj)->submitter->otherFields = initializeList(printField, deleteField, compareFields);
	//initialize Lists
	(*obj)->individuals = initializeList(printIndividual, deleteIndividual, compareIndividuals);
	(*obj)->families = initializeList(printFamily, deleteFamily, compareFamilies);

}

bool checkTags (char * tagContainer[], char * tag) {
	int i = 0;
	
	for (i = 0; tagContainer[i] != NULL; i++) {
		if (strcmp(tagContainer[i], tag) == 0) {
			return true;
		}
	}
	
	return false;
}

Family * createFamily () {
	Family * f = calloc(1, sizeof(Family));
	f->wife = NULL;
	f->husband = NULL;
	f->children = initializeList(printIndividual, deleteIndividual, compareIndividuals);
	f->otherFields = initializeList(printField, deleteField, compareFields);
	f->events = initializeList(printEvent, deleteEvent, compareEvents);
	return f;
}

Individual * createIndividual () {
	Individual * i = calloc(1, sizeof(Individual));
	i->givenName = NULL;
	i->surname = NULL;
	i->events = initializeList(printEvent, deleteEvent, compareEvents);
	i->families = initializeList(printFamily, deleteFamily, compareFamilies);
	i->otherFields = initializeList(printField, deleteField, compareFields);
	
	return i;
}

Event * createEvent () {
	Event * e = calloc(1, sizeof(Event));
	e->date = NULL;
	e->place = NULL;
	e->otherFields = initializeList(printField, deleteField, compareFields);
	
	return e;
}


Field * createField () {
	Field * f = calloc(1, sizeof(Field));
	f->tag = NULL;
	f->value = NULL;
	
	return f;
}


char * combineString (char ** string, int first, int second) {
	
	int initial = first - 1; //2
	int final = second; //4
	int size = 0;
	
	if (string[initial] == NULL || string[final - 1] == NULL || first < 1) {
		return NULL;
	}
	//used to combine tokenized strings back to their original form!
	if (first == second) {
		char * combinedString = calloc(1, sizeof(char) * (strlen(string[first-1]) + 255));
		strcpy(combinedString, string[first-1]);
		return combinedString;
	}
	
	char * combinedString = NULL;
	
	for (int i = initial; i < final; i++) {
		size += (sizeof(string[i]) + 2);
	}
	
	combinedString = calloc(size + 255, sizeof(char));
	
	strcpy(combinedString, "");
	
	for (int i = initial; i < final; i++) {
		strcat(combinedString, string[i]);
		strcat(combinedString, " ");
	}
	
	if (combinedString[strlen(combinedString) - 1] == ' ') {
		combinedString[strlen(combinedString) - 1] = '\0';
	}
	
	return combinedString;
	
}

GEDCOMerror regularParser (stringInformation splitString, GEDCOMobject * obj, int counter, recordSet * currentRecord, subSet * subRecord, bool isToPointer, List * addressList, List * recieveAddress, int * checkHeader) {
	
	
	int size = splitString.size;
	char ** tokenizedString = splitString.tokenizedString;
	
	if (atoi(tokenizedString[0]) == 1) {
		*subRecord = NOSUB;
	}
	
	if(strcmp(tokenizedString[0], "0") == 0) {
		//checks if level 0 is a valid tag
		if(checkTags(recordTags, tokenizedString[1])) {
/***********************RECORD CREATION*******************************/

			if (strcmp(tokenizedString[1], "HEAD") == 0) {			
				*currentRecord = (recordSet)HEADER;
			}	
			else if (strcmp(tokenizedString[1], "SUBM") == 0) {
				if (!checkHeaderInit(obj->header, checkHeader)) return createError(INV_HEADER, counter - 1);
				*currentRecord = (recordSet)SUBMITTER;
			}
			else if (strcmp(tokenizedString[1], "INDI") == 0) {
				if (!checkHeaderInit(obj->header, checkHeader)) return createError(INV_HEADER, counter - 1);
				insertBack(&(obj->individuals), createIndividual());
				*currentRecord = (recordSet)INDIVIDUAL;
			}
			else if (strcmp(tokenizedString[1], "FAM") == 0) {
				if (!checkHeaderInit(obj->header, checkHeader)) return createError(INV_HEADER, counter - 1);
				insertBack(&(obj->families), createFamily());
				*currentRecord = (recordSet)FAMILY;
			}
			
		}
		else {
			//Invalid record tag!
			return createError(INV_RECORD, counter);
		}
	}
	else {
		//Level is greater than 0
		if (*currentRecord == HEADER) {
			//only checks for header tags				
			if(checkTags(headerTags, tokenizedString[1])) {
				//if current line is the source of the header!
				if (strcmp(tokenizedString[1], "SOUR") == 0) {
					//if no source value was entered
					if (size < 3) {
						return createError(INV_RECORD, counter);
					}
					
					//combine the remaining tokens to create a large string
					char * tempSource = NULL;
					tempSource = combineString(tokenizedString, 3, size);
					
					
						//only take a max length of 61
					if (tempSource != NULL && strlen(tempSource) < 250) {
						
						//printf("test\n");
						strcpy(((*obj).header)->source, tempSource);
						//printf("%s\n", ((*obj).header)->source);
						free(tempSource);
						
						
					}
					else {
						//return error if greater than 61
						free(tempSource);
						return createError(INV_RECORD, counter);
					}
					
				}
				else if (strcmp(tokenizedString[1], "GEDC") == 0) {
					*subRecord = GEDC;
				}
				else if (strcmp(tokenizedString[1], "SUBM") == 0) {
					if (isToPointer == true) {
						insertBack(recieveAddress, createAddress( NULL, &(obj->header->submitter), NULL, tokenizedString[2], NULL, counter ) );
					}
					else {
						*subRecord = SUBM;
					}
					
					*checkHeader = 1;
				}
			}
			else {
				//may also need to be removed!
				if (size < 3) {
					return createError(INV_RECORD, counter);
				}
				
				if (strcmp(tokenizedString[1], "VERS") == 0 && *subRecord == GEDC) {
					//printf("test\n");
					
					if (strtol(tokenizedString[2], NULL, 10) == 0) {
						return createError(INV_RECORD, counter);
					}
					obj->header->gedcVersion = atof(tokenizedString[2]);
					
				}
				else if (strcmp(tokenizedString[1], "CHAR") == 0) {
					if (strcmp(tokenizedString[2], "ANSEL")) {
						obj->header->encoding = ANSEL;
					}
					else if (strcmp(tokenizedString[2], "UTF8")) {
						obj->header->encoding = UTF8;
					}
					else if (strcmp(tokenizedString[2], "UNICODE")) {
						obj->header->encoding = UNICODE;
					}
					else if (strcmp(tokenizedString[2], "ASCII")) {
						obj->header->encoding = ASCII;
					}
				}
				else if (strcmp(tokenizedString[1], "NAME") == 0 && *subRecord == SUBM) {
					//for  header -> submitters
					char * temp;
					temp = combineString(tokenizedString, 3, size);
					if (temp != NULL && (strlen(temp) < 62)) {
						strcpy(obj->header->submitter->submitterName, temp);
						free(temp);
					}
					else {
						if (temp!=NULL) free(temp);
						return createError(INV_RECORD, counter);
					}
					//if (temp != NULL) free(temp);
					//you can add other flags for other records later!
				}
				else if (strcmp(tokenizedString[1], "ADDR") == 0 && *subRecord == SUBM) {
					if (*subRecord == SUBM) {
						
						char * temp = combineString(tokenizedString, 3, size);
						
						if (temp != NULL && (strlen(temp) < 250)) {
							strcpy(obj->header->submitter->address, temp);
							free(temp);
						}						
						else {
							if (temp==NULL) {
								free(temp);
							}
							return createError(INV_RECORD, counter);
						}
					} 
				}
				else {
					
					if (*subRecord == SUBM) {
						
						//obj->header->submitter->otherFields = initializeList(printField, deleteField, compareFields);
						
						insertBack(&(obj->header->submitter->otherFields), createField());

						((Field*)getFromBack((obj->header->submitter->otherFields)))->tag = calloc(1, (strlen(tokenizedString[1]) + 1) * sizeof(char));
						strcpy(((Field*)getFromBack((obj->header->submitter->otherFields)))->tag, tokenizedString[1]);
					

						//((Field*)getFromBack((obj->header->submitter->otherFields)))->value = calloc(1,sizeof(char*));
						((Field*)getFromBack((obj->header->submitter->otherFields)))->value = combineString(tokenizedString, 3, size);
						//printf("%s\n", ((Field*)getFromBack((obj->header->submitter->otherFields)))->value);
						
						
					}
					else {
						
						insertBack(&(obj->header->otherFields), createField());
					
						((Field*)getFromBack((obj->header->otherFields)))->tag = calloc(1,sizeof(tokenizedString[1]));
						strcpy(((Field*)getFromBack((obj->header->otherFields)))->tag, tokenizedString[1]);
					

						//((Field*)getFromBack((obj->header->otherFields)))->value = calloc(1,sizeof(char*));
						((Field*)getFromBack((obj->header->otherFields)))->value = combineString(tokenizedString, 3, size);
						//printf("%s\n", ((Field*)getFromBack((obj->header->otherFields)))->value);
						
					}
					
					
				}
			//IMPORTANT : might need to clarify whether to return error or
				//just shove into others field!
				//return createError(INV_RECORD, counter);
			}
				
			//printf("line %d : current record is set to Header.\n", counter);
		}
		else if (*currentRecord == SUBMITTER) {
			//printf("line %d : current record is set to Submitter.\n", counter);
			if (strcmp(tokenizedString[1], "NAME") == 0 && *subRecord == NOSUB) {
				//for  header -> submitters
				char * temp;
				temp = combineString(tokenizedString, 3, size);
				if (temp != NULL && (strlen(temp) < 62)) {
					strcpy(obj->submitter->submitterName, temp);
					free(temp);
				}
				else {
					if (temp!=NULL) free(temp);
					return createError(INV_RECORD, counter);
				}
				//if (temp != NULL) free(temp);
				//you can add other flags for other records later!
			}
			else if (strcmp(tokenizedString[1], "ADDR") == 0 && *subRecord == SUBM) {
				if (*subRecord == SUBM) {
					
					char * temp = combineString(tokenizedString, 3, size);
					
					if (temp != NULL && (strlen(temp) < 250)) {
						strcpy(obj->submitter->address, temp);
						free(temp);
						temp = NULL;
					}						
					else {
						if (temp==NULL) {
							free(temp);
							temp = NULL;
						}
						return createError(INV_RECORD, counter);
					}
				} 
			}
			else {
				//all others
				//obj->submitter->otherFields = initializeList(printField, deleteField, compareFields);
				insertBack(&(obj->submitter->otherFields), createField());

				((Field*)getFromBack((obj->submitter->otherFields)))->tag = calloc(1,(strlen(tokenizedString[1]) + 1) * sizeof(char));
				strcpy(((Field*)getFromBack((obj->submitter->otherFields)))->tag, tokenizedString[1]);
					

				((Field*)getFromBack((obj->submitter->otherFields)))->value = combineString(tokenizedString, 3, size);
			}
		}
		else if (*currentRecord == INDIVIDUAL) {
			Individual * i = (Individual*)getFromBack((obj->individuals));
			
			
			if (strcmp(tokenizedString[1], "NAME") == 0) {
				if (size < 4) {
					if (strlen(tokenizedString[2]) < 3 && tokenizedString[2][0] == '/' && tokenizedString[2][1] == '/') {
						i->givenName = NULL;
						i->surname = NULL;
					}
					else if (tokenizedString[2][0] == '/' && tokenizedString[2][strlen(tokenizedString[2]) - 1] == '/') {
						if (i->surname == NULL && i->givenName == NULL) {
							i->givenName = NULL;
						
							i->surname = calloc(1, (strlen(tokenizedString[2]) + 1) * sizeof(char));
							strncpy(i->surname, &(tokenizedString[2][1]), strlen(tokenizedString[2]) - 2);
						}
						else if (i->surname == NULL) {
							i->surname = calloc(1,(strlen(tokenizedString[2]) + 1) * sizeof(char));
							strncpy(i->surname, &(tokenizedString[2][1]), strlen(tokenizedString[2]) - 2);
						}
						
					}
					else {
						return createError(INV_RECORD, counter);
					}
				}
				else if (size == 4) {
					if (i->givenName == NULL) {
						i->givenName = calloc(1,(strlen(tokenizedString[2]) + 1) * sizeof(char));
						strcpy(i->givenName, tokenizedString[2]);
					}
					if (i->surname == NULL) {
						i->surname = calloc(1,(strlen(tokenizedString[3]) + 1) * sizeof(char));
						
						if (tokenizedString[3][0] == '/' && tokenizedString[3][strlen(tokenizedString[3]) - 1] == '/') {
							strncpy(i->surname, &(tokenizedString[3][1]), strlen(tokenizedString[3]) - 2);
						}
						else if (tokenizedString[3][0] == '/' || tokenizedString[3][strlen(tokenizedString[3]) - 1] == '/'){
							return createError(INV_RECORD, counter);
						}
						else {
							return createError (INV_RECORD, counter);
						}
					}
				}
			}
			else if (strcmp(tokenizedString[1], "GIVN") == 0) {
				if (i->givenName == NULL) {
					if (size < 3) {
						return createError(INV_RECORD, counter);
					}
					else {
						i->givenName = calloc(1,(strlen(tokenizedString[2	]) + 1) * sizeof(char));
						strcpy(i->givenName, tokenizedString[2]);
					}
				}
			}
			else if (strcmp(tokenizedString[1], "SURN") == 0) { 
				if (i->surname == NULL) {
					
					if (size < 3) {
						return createError(INV_RECORD, counter);
					}
					else {
						i->surname = calloc(1,(strlen(tokenizedString[2	]) + 1) * sizeof(char));
						strcpy(i->surname, tokenizedString[2]);
					}
				}
			}
			else if (checkTags(eventTags, tokenizedString[1])) {
				*subRecord = EVEN;
				insertBack(&(i->events), createEvent());
				Event * e = getFromBack(i->events);
				strcpy(e->type, tokenizedString[1]);
				
			}
			else if (*subRecord == EVEN) {
				if (strcmp(tokenizedString[1], "DATE") == 0) {
					Event * e = getFromBack(i->events);
					e->date = combineString(tokenizedString, 3, size);
					
					
				}
				else if (strcmp(tokenizedString[1], "PLAC") == 0) {
					Event * e = getFromBack(i->events);
					e->place = combineString(tokenizedString, 3, size);
					
				}
				else { //otherFields
					Event * e = getFromBack(i->events);
					insertBack(&(e->otherFields), createField());
					Field * f = getFromBack(e->otherFields);
					f->tag = calloc(1,(strlen(tokenizedString[1])+1) * sizeof(char));
					strcpy(f->tag, tokenizedString[1]);
					f->value = combineString(tokenizedString, 3, size);
				}
			}
			else {
				if (isToPointer == true) {
					//remove the null when u figure out what to put in there!
					insertBack(recieveAddress, createAddress((&(i->families)), NULL, NULL, tokenizedString[2], NULL, counter));
				}
				else if (size > 2) {
					insertBack(&(i->otherFields), createField());
					Field * f = getFromBack(i->otherFields);
					f->tag = calloc(1,(strlen(tokenizedString[1])+1) * sizeof(char));
					strcpy(f->tag, tokenizedString[1]);
					f->value = combineString(tokenizedString, 3, size);
				}
				else {
					return createError(INV_RECORD, counter);
				}
			}
		}
		else if (*currentRecord == FAMILY) {
			
			Family * i = (Family*)getFromBack((obj->families));
			
			if (checkTags(feventTags, tokenizedString[1])) {
				*subRecord = EVEN;
				insertBack(&(i->events), createEvent());
				Event * e = getFromBack(i->events);
				strcpy(e->type, tokenizedString[1]);
				
			}
			else if (*subRecord == EVEN) {
				if (strcmp(tokenizedString[1], "DATE") == 0) {
					Event * e = getFromBack(i->events);
					e->date = combineString(tokenizedString, 3, size);
					
				}
				else if (strcmp(tokenizedString[1], "PLAC") == 0) {
					Event * e = getFromBack(i->events);
					e->place = combineString(tokenizedString, 3, size);
				}
				else { 
					//otherFields
					Event * e = getFromBack(i->events);
					insertBack(&(e->otherFields), createField());
					Field * f = getFromBack(e->otherFields);
					f->tag = calloc(1,strlen(tokenizedString[1]) * sizeof(char) + 1);
					f->value = combineString(tokenizedString, 3, size);
				}
			}
			else {
				if (isToPointer == true) {
					//remove the null when u figure out what to put in there!
					if (strcmp(tokenizedString[1], "HUSB") == 0) {
						insertBack(recieveAddress, createAddress(NULL, NULL, &(i->husband), tokenizedString[2], NULL, counter));
					}
					else if (strcmp(tokenizedString[1], "WIFE") == 0) {
						insertBack(recieveAddress, createAddress(NULL, NULL, &(i->wife), tokenizedString[2], NULL, counter));
					}
					else if (strcmp(tokenizedString[1], "CHIL") == 0){
						insertBack(recieveAddress, createAddress(&(i->children), NULL, NULL, tokenizedString[2], NULL, counter));
					}
					else {
						return createError(INV_RECORD, counter);
					}
				}
				else { 
					//otherFields
					insertBack(&(i->otherFields), createField());
					Field * f = getFromBack(i->otherFields);
					f->tag = calloc(1,strlen(tokenizedString[1]) * sizeof(char) + 1);
					strcpy(f->tag, tokenizedString[1]);
					f->value = combineString(tokenizedString, 3, size);
				}
			}
		}
			
	}
	
	return createError(OK, -1);	
}

GEDCOMerror pointerParser(stringInformation splitString, GEDCOMobject * obj, int counter, recordSet * currentRecord, subSet * subRecord, List * addressList, int * checkHeader) {
	
	int size = splitString.size;
	char ** tokenizedString = splitString.tokenizedString;
	
	if (size < 3) {
			//level and pointer only, invalid format!
			return createError(INV_RECORD, counter);
	}
		
	if(strcmp(tokenizedString[0], "0") == 0) {
		
		if(checkTags(recordTags, tokenizedString[2])) {
			
			//create Record
			if (strcmp(tokenizedString[2], "HEAD") == 0) {
				*currentRecord = (recordSet)HEADER;
			}
			else if (strcmp(tokenizedString[2], "SUBM") == 0) {
				if (!checkHeaderInit(obj->header, checkHeader)) return createError(INV_HEADER, counter - 1);
				*currentRecord = (recordSet)SUBMITTER;
				insertBack(addressList, createAddress(NULL, &(obj->submitter), NULL , tokenizedString[1], NULL, counter));
			}
			else if (strcmp(tokenizedString[2], "INDI") == 0) {
				if (!checkHeaderInit(obj->header, checkHeader)) return createError(INV_HEADER, counter - 1);
				insertBack(&(obj->individuals), createIndividual());
				insertBack(addressList, createAddress(NULL, NULL, NULL, tokenizedString[1], getFromBack(obj->individuals), counter));
				*currentRecord = (recordSet)INDIVIDUAL;
			}
			else if (strcmp(tokenizedString[2], "FAM") == 0) {
				if (!checkHeaderInit(obj->header, checkHeader)) return createError(INV_HEADER, counter - 1);
				insertBack(&(obj->families), createFamily());
				insertBack(addressList, createAddress(NULL, NULL, NULL, tokenizedString[1], getFromBack(obj->families), counter));
				*currentRecord = (recordSet)FAMILY;
			}
		}
		else {
			//Invalid record tag!
			return createError(INV_RECORD, counter);
		}
		//printf("Valid tag.\n");
	}
	else {
		return createError(INV_RECORD, counter);
	}
		
	return createError(OK, -1);
}


void deleteAddress(void* toBeDeleted) {
	
}

int compareAddress(const void* first,const void* second) {
	
	return strcmp(((char*)first), ((char*)second));
	
}

char* printAddress(void* toBePrinted) {
	return NULL;
}

addressInformation * createAddress (List * listPointer, Submitter ** submitterPointer, Individual ** spousePointer, char * address, void * initializedPointer, int count) {
	addressInformation * a = calloc(1,sizeof(addressInformation));
	a->address = calloc(1,(strlen(address) + 1) * sizeof(char));
	
	if (listPointer != NULL) {
		a->listPointer = listPointer;
		a->type = 0;
	}
	if (submitterPointer != NULL) {
		a->submitterPointer = submitterPointer;
		a->type = 1;
	}
	if (spousePointer != NULL) {
		a->spousePointer = spousePointer;
		a->type = 2;
	}
	if (initializedPointer != NULL) {
		a->initializedPointer = initializedPointer;
		a->type = 3;
	}
	
	a->count = count;

	strcpy(a->address, address);
	
	return a;
}

bool findInLinker(List addressList, void * data) {
	
	addressInformation * a = data;
	
	char * address = a->address;
	
	
	Node * n = addressList.head;
	
	while (n!=NULL) {
		addressInformation * b = n->data;
		if (compareAddress(b->address, address) == 0) {
			//printf("%d\n", ((addressInformation*)(n->data))->type);
			
			if (a->type == 0) { // List *
				
				if (b->type == 3) { // Void * 
					List * l = a->listPointer;
					insertBack(l, b->initializedPointer);
					return true;
				}
				
			}
			else if (a->type == 1) { // Submitter *
				
				if (b->type == 1) { // Submitter *
					
					*(a->submitterPointer) = *(b->submitterPointer);
					return true;
				}				
			}
			else if (a->type == 2) { // Individual *
				
				if (b->type == 3) { // Void * 
					*(a->spousePointer) = b->initializedPointer;
					return true;
				}
			}
		}
		n=n->next;
	}
	
	return false;
}

void freeLists (List addressList, List recieveList) {
	Node * n = addressList.head;
	Node * delete = NULL;
	
	while (n!=NULL) {
		addressInformation * a = n->data;
		
		free(a->address);
		free(a);
		delete = n;
		n = n->next;
		free(delete);
	}
	
	n = recieveList.head;
	
	while (n!=NULL) {
		addressInformation * a = n->data;
		
		free(a->address);
		free(a);
		delete = n;
		n = n->next;
		free(delete);
	}
	
	
}

bool compareFindPerson(const void* first,const void* second) {
	
	char temp[1024];
	
	strcpy(temp, ((Individual*)first)->givenName);
	strcat(temp, ", ");
	strcat(temp, ((Individual*)first)->surname);
	
	char secondTemp[1024];
	
	strcpy(secondTemp, ((Individual*)second)->givenName);
	strcat(secondTemp, ", ");
	strcat(secondTemp, ((Individual*)second)->surname);
	
	if (strcmp(temp, secondTemp) == 0) {
		return true;
	}
	else {
		return false;
	}

}

bool checkHeaderInit(Header * h, int * submitterFlag) {
	
	if (h == NULL) {
		return false;
	}
	else if (h->gedcVersion < 0) {

		return false;
	}
	else if (h->source == NULL) {
		return false;
	}
	else if (*submitterFlag == 0) {
		return false;
	}
	
	
	return true;
}

bool isAlpha (char * string) {
	int sie = strlen(string);
	
	for (int i = 0; i < sie; i ++) {
		if (!isupper(string[i])) {
			return false;
		}
	}
	
	return true;
}

Individual * createCopy(Individual * input) {
	if (input == NULL) {
		return NULL;
	}
	
	Individual * i = malloc(sizeof(Individual));
	if (input->givenName != NULL) {
		i->givenName = malloc(strlen(input->givenName) * 2);
		strcpy(i->givenName, input->givenName);
	}
	
	if (input->surname != NULL) {
		i->surname = malloc(strlen(input->surname) * 2);
		strcpy(i->surname, input->surname);
	}
	
	i->otherFields = initializeList(printField, deleteField, compareFields);
	i->events = initializeList(printEvent, deleteEvent, compareEvents);
	i->families = initializeList(printFamily, deleteFamily, compareFamilies);
	
	return i;
	
}









