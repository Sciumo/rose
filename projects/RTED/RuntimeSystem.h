#ifndef RTEDRUNTIME_H
#define RTEDRUNTIME_H
#include <stdio.h>
//#include <cstdio>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************** HELPER FUNCTIONS *************************************/
const char* RuntimeSystem_roseConvertIntToString(int t);

// USE GUI for debugging
void Rted_debugDialog(const char* filename, int line, int lineTransformed);

void RuntimeSystem_roseCheckpoint( const char* filename, const char* line, const char* lineTransformed );
/***************************** HELPER FUNCTIONS *************************************/



/***************************** ARRAY FUNCTIONS *************************************/
void RuntimeSystem_roseCreateArray(const char* name, const char* mangl_name,
		const char* type, const char* basetype, size_t indirection_level,
		unsigned long int address, long int size, long int mallocSize,
		const char* class_name, const char* filename, const char* line,
		const char* lineTransformed, int dimensions, ...);

void RuntimeSystem_roseArrayAccess(const char* filename,
		unsigned long int base_address, // &( array[ 0 ])
		unsigned long int address, long int size, int read_write_mask, // 1 = read, 2 = write
		const char* line, const char* lineTransformed);
/***************************** ARRAY FUNCTIONS *************************************/



/***************************** FUNCTION CALLS *************************************/

void RuntimeSystem_roseAssertFunctionSignature(
		const char* filename, const char* line, const char* lineTransformed,
		const char* name, int type_count, ... );

void RuntimeSystem_roseConfirmFunctionSignature(
		const char* name, int type_count, ... );

void RuntimeSystem_handleSpecialFunctionCalls(const char* funcname,
		const char** args, int argsSize, const char* filename,
		const char* line, const char* lineTransformed, const char* stmtStr,
		const char* leftHandSideVar);

void RuntimeSystem_roseIOFunctionCall(const char* funcname,
		const char* filename, const char* line, const char* lineTransformed,
		const char* stmtStr, const char* leftHandSideVar, FILE* file,
		const char* arg1, const char* arg2);

void RuntimeSystem_roseFunctionCall(int count, ...);
/***************************** FUNCTION CALLS *************************************/



/***************************** MEMORY FUNCTIONS *************************************/
void RuntimeSystem_roseFreeMemory(void* ptr, const char* filename,
		const char* line, const char* lineTransformed);

void RuntimeSystem_roseReallocateMemory(void* ptr, unsigned long int size,
		const char* filename, const char* line, const char* lineTransformed);

void RuntimeSystem_checkMemoryAccess(unsigned long int address, long int size,
		int read_write_mask);
/***************************** MEMORY FUNCTIONS *************************************/



/***************************** SCOPE *************************************/
// handle scopes (so we can detect when locals go out of scope, free up the
// memory and possibly complain if the local was the last var pointing to some
// memory)
void RuntimeSystem_roseEnterScope(const char* scope_name);
void RuntimeSystem_roseExitScope(const char* filename, const char* line, const char* lineTransformed,
		const char* stmtStr);
/***************************** SCOPE *************************************/


void RuntimeSystem_roseRtedClose();

// function used to indicate error
void RuntimeSystem_callExit(const char* filename, const char* line,
		const char* reason, const char* stmtStr);

extern int RuntimeSystem_original_main(int argc, char**argv, char**envp);
/***************************** INIT AND EXIT *************************************/



/***************************** VARIABLES *************************************/

// TODO 1 djh: comment
int RuntimeSystem_roseCreateVariable(const char* name,
		const char* mangled_name, const char* type, const char* basetype,
		size_t indirection_level, unsigned long int address, unsigned int size,
		int init, const char* className, const char* filename,
		const char* line, const char* lineTransformed);

int RuntimeSystem_roseInitVariable(const char* typeOfVar2,
		const char* baseType2, size_t indirection_level,
		const char* class_name, unsigned long long address, unsigned int size,
		int ismalloc, int pointer_changed, const char* filename,
		const char* line, const char* lineTransformed);

void RuntimeSystem_roseMovePointer(
                unsigned long long address,
                const char* type,
                const char* base_type,
                size_t indirection_level,
                const char* class_name,
                const char* filename,
                const char* line,
                const char* lineTransformed);

void RuntimeSystem_roseAccessVariable(
        unsigned long long address,
		unsigned int size, 
        unsigned long long write_address, 
        unsigned int write_size,
        int read_write_mask, //1 = read, 2 = write
        const char* filename, const char* line,
		const char* lineTransformed);
/***************************** VARIABLES *************************************/



/***************************** TYPES *************************************/
// handle structs and classes
void RuntimeSystem_roseRegisterTypeCall(int count, ...);
/***************************** TYPES *************************************/




#ifdef __cplusplus
} // extern "C"
#endif

#endif

