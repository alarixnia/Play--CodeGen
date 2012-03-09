#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "MemoryFunction.h"

#ifdef WIN32

#include <windows.h>

#ifdef _M_X64
extern "C" void _CMemoryFunction_Execute(void*, void*);
#endif

#elif defined(__APPLE__)

#include "TargetConditionals.h"
#include <mach/mach_init.h>
#include <mach/vm_map.h>
extern "C" void __clear_cache(void* begin, void* end);

#endif

CMemoryFunction::CMemoryFunction(const void* code, size_t size)
: m_code(NULL)
{
#ifdef WIN32
	m_size = size;
	m_code = malloc(size);
	memcpy(m_code, code, size);
	
	DWORD oldProtect = 0;
	BOOL result = VirtualProtect(m_code, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	assert(result == TRUE);
#elif defined(__APPLE__)
	vm_size_t page_size = 0;
	host_page_size(mach_task_self(), &page_size);
	unsigned int allocSize = ((size + page_size - 1) / page_size) * page_size;
	vm_allocate(mach_task_self(), reinterpret_cast<vm_address_t*>(&m_code), allocSize, TRUE); 
	memcpy(m_code, code, size);
	__clear_cache(m_code, reinterpret_cast<uint8*>(m_code) + size);
	kern_return_t result = vm_protect(mach_task_self(), reinterpret_cast<vm_address_t>(m_code), size, 0, VM_PROT_READ | VM_PROT_EXECUTE);
	assert(result == 0);
	m_size = allocSize;
#endif
}

CMemoryFunction::~CMemoryFunction()
{
    if(m_code != NULL)
    {
#ifdef WIN32
		free(m_code);
#elif defined(__APPLE__)
		vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(m_code), m_size); 
#endif
    }
}

void CMemoryFunction::operator()(void* context)
{

#ifdef WIN32
	
	#ifdef _M_X64
	
		_CMemoryFunction_Execute(m_code, context);
	
	#else
	
		volatile const void* code = m_code;
		volatile const void* dataPtr = context;
	
		__asm
		{
			push ebp
			push ebx
			push esi
			push edi

			mov eax, code
			mov ebp, dataPtr

			mov edi, esp
			sub esp, 0x10
			and esp, ~0xF
			sub esp, 0x08
			push edi

			call eax

			pop esp

			pop edi
			pop esi
			pop ebx
			pop ebp
		}

	#endif
	
#elif defined(__APPLE__)
	
	volatile const void* code = m_code;
	volatile const void* dataPtr = context;
	
	#if TARGET_CPU_ARM
			
		__asm__ ("mov r1, %0" : : "r"(dataPtr) : "r1");
		__asm__ ("mov r0, %0" : : "r"(code) : "r0");
		__asm__ ("stmdb sp!, {r2, r3, r4, r5, r6, r7, r11, ip, lr}");
		__asm__ ("mov r11, r1");
		__asm__ ("blx r0");
		__asm__ ("ldmia sp!, {r2, r3, r4, r5, r6, r7, r11, ip, lr}");
	
	#else // TARGET_CPU_X86
	
		#if TARGET_RT_64_BIT
	
			__asm__("mov %0, %%rcx\n"
					"mov %1, %%rdx\n"
					"call *%%rcx\n"
					: : "r"(code), "r"(dataPtr) : "%rax", "%rcx", "%rdx");
	
		#else
	
			__asm__("push %%ebp\n"
					"push %%ebx\n"
					"push %%esi\n"
					"push %%edi\n"
					"mov %0, %%eax\n"
					"mov %1, %%ebp\n"
					
					"mov %%esp, %%edi\n"
					"sub $0x10, %%esp\n"
					"and $~0xF, %%esp\n"
					"sub $0x08, %%esp\n"
					"push %%edi\n"
					
					"call *%%eax\n"
					
					"pop %%esp\n"
					"pop %%edi\n"
					"pop %%esi\n"
					"pop %%ebx\n"
					"pop %%ebp\n"
					: : "r"(code), "r"(dataPtr) : "%eax", "%ecx", "%edx");
	
		#endif
	
	#endif
	
#endif

}

void* CMemoryFunction::GetCode() const
{
	return m_code;
}

size_t CMemoryFunction::GetSize() const
{
	return m_size;
}
