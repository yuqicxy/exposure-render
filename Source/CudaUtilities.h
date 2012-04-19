/*
	Copyright (c) 2011, T. Kroes <t.kroes@tudelft.nl>
	All rights reserved.

	Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

	- Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
	- Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
	- Neither the name of the TU Delft nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
	
	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <cuda.h>
#include <cuda_runtime_api.h>
#include <map>

#include "Exception.h"

using namespace std;

namespace ExposureRender
{

class CCudaTimer
{
public:
	CCudaTimer(void);
	virtual ~CCudaTimer(void);

	void	StartTimer(void);
	float	StopTimer(void);
	float	ElapsedTime(void);

private:
	bool			m_Started;
	cudaEvent_t 	m_EventStart;
	cudaEvent_t 	m_EventStop;
};

class CUDA
{
public:
	static void HandleCudaError(const cudaError_t& CudaError, const char* pTitle = "")
	{
		char Message[256];

		sprintf_s(Message, 256, "%s (%s)", cudaGetErrorString(CudaError), pTitle);

		if (CudaError != cudaSuccess)
			throw(Exception(Enums::Error, Message));
	}

	static void ThreadSynchronize()
	{
		CUDA::HandleCudaError(cudaThreadSynchronize(), "cudaThreadSynchronize");
	}

	template<class T> static void Allocate(T*& pDevicePointer, int Num = 1)
	{
		CUDA::ThreadSynchronize();
		HandleCudaError(cudaMalloc((void**)&pDevicePointer, Num * sizeof(T)), "cudaMalloc");
		CUDA::ThreadSynchronize();
	}

	template<class T> static void AllocatePiched(T*& pDevicePointer, const int Pitch, const int Width, const int Height)
	{
		CUDA::ThreadSynchronize();
		HandleCudaError(cudaMallocPitch((void**)&pDevicePointer, (size_t*)&Pitch, Width * sizeof(T), Height), "cudaMallocPitch");
		CUDA::ThreadSynchronize();
	}
	
	template<class T> static void MemSet(T*& pDevicePointer, const int Value, int Num = 1)
	{
		CUDA::ThreadSynchronize();
		HandleCudaError(cudaMemset((void*)pDevicePointer, Value, (size_t)(Num * sizeof(T))), "cudaMemset");
		CUDA::ThreadSynchronize();
	}

	template<class T> static void HostToConstantDevice(T* pHost, char* pDeviceSymbol, int Num = 1)
	{
		CUDA::ThreadSynchronize();
		HandleCudaError(cudaMemcpyToSymbol(pDeviceSymbol, pHost, Num * sizeof(T)), "cudaMemcpyToSymbol");
		CUDA::ThreadSynchronize();
	}

	template<class T> static void MemCopyHostToDeviceSymbol(T* pHost, char* pDeviceSymbol, int Num = 1)
	{
		CUDA::ThreadSynchronize();
		HandleCudaError(cudaMemcpyToSymbol(pDeviceSymbol, pHost, Num * sizeof(T)), "cudaMemcpyToSymbol");
		CUDA::ThreadSynchronize();
	}

	template<class T> static void MemCopyDeviceToDeviceSymbol(T* pDevice, char* pDeviceSymbol, int Num = 1)
	{
		CUDA::ThreadSynchronize();
		HandleCudaError(cudaMemcpyToSymbol(pDeviceSymbol, pDevice, Num * sizeof(T), 0, cudaMemcpyDeviceToDevice), "cudaMemcpyToSymbol");
		CUDA::ThreadSynchronize();
	}

	template<class T> static void MemCopyHostToDevice(T* pHost, T* pDevice, int Num = 1)
	{
		CUDA::ThreadSynchronize();
		HandleCudaError(cudaMemcpy(pDevice, pHost, Num * sizeof(T), cudaMemcpyHostToDevice), "cudaMemcpy");
		CUDA::ThreadSynchronize();
	}

	template<class T> static void MemCopyDeviceToHost(T* pDevice, T* pHost, int Num = 1)
	{
		CUDA::ThreadSynchronize();
		HandleCudaError(cudaMemcpy(pHost, pDevice, Num * sizeof(T), cudaMemcpyDeviceToHost), "cudaMemcpy");
		CUDA::ThreadSynchronize();
	}

	template<class T> static void MemCopyDeviceToDevice(T* pDeviceSource, T* pDeviceDestination, int Num = 1)
	{
		CUDA::ThreadSynchronize();
		HandleCudaError(cudaMemcpy(pDeviceDestination, pDeviceSource, Num * sizeof(T), cudaMemcpyDeviceToDevice), "cudaMemcpy");
		CUDA::ThreadSynchronize();
	}

	static void FreeArray(cudaArray*& pCudaArray)
	{
		CUDA::ThreadSynchronize();
		HandleCudaError(cudaFreeArray(pCudaArray), "cudaFreeArray");
		pCudaArray = NULL;
		CUDA::ThreadSynchronize();
	}

	template<class T> static void Free(T*& pBuffer)
	{
		if (pBuffer == NULL)
			return;

		CUDA::ThreadSynchronize();
		
		HandleCudaError(cudaFree(pBuffer), "cudaFree");
		pBuffer = NULL;

		CUDA::ThreadSynchronize();
	}

	static void UnbindTexture(textureReference& pTextureReference)
	{
		CUDA::ThreadSynchronize();
		HandleCudaError(cudaUnbindTexture(&pTextureReference), "cudaUnbindTexture");

		CUDA::ThreadSynchronize();
	}

	template<class T> static void BindTexture1D(textureReference& TextureReference, int Num, const T* pBuffer, cudaArray*& pCudaArray, cudaTextureFilterMode TextureFilterMode = cudaFilterModeLinear, cudaTextureAddressMode TextureAddressMode = cudaAddressModeClamp, bool Normalized = true)
	{
		CUDA::ThreadSynchronize();

		const cudaChannelFormatDesc ChannelDescription = cudaCreateChannelDesc<T>();

		TextureReference.normalized		= Normalized;
		TextureReference.filterMode		= TextureFilterMode;
		TextureReference.addressMode[0]	= TextureAddressMode;

		CUDA::FreeArray(pCudaArray);

		HandleCudaError(cudaMallocArray(&pCudaArray, &ChannelDescription, Num, 1));
		HandleCudaError(cudaMemcpyToArray(pCudaArray, 0, 0, pBuffer, Num * sizeof(T), cudaMemcpyHostToDevice));
		HandleCudaError(cudaBindTextureToArray(&TextureReference, pCudaArray, &ChannelDescription));

		CUDA::ThreadSynchronize();
	}

	template<class T> static void BindTexture3D(textureReference& TextureReference, int Extent[3], const T* pBuffer, cudaArray*& pCudaArray, cudaTextureFilterMode TextureFilterMode = cudaFilterModeLinear, cudaTextureAddressMode TextureAddressMode = cudaAddressModeClamp, bool Normalized = true)
	{
		CUDA::ThreadSynchronize();

		const cudaChannelFormatDesc ChannelDescription = cudaCreateChannelDesc<T>();

		const cudaExtent CudaExtent = make_cudaExtent(Extent[0], Extent[1], Extent[2]);

		HandleCudaError(cudaMalloc3DArray(&pCudaArray, &ChannelDescription, CudaExtent));

		cudaMemcpy3DParms CopyParams = {0};

		CopyParams.srcPtr		= make_cudaPitchedPtr((void*)pBuffer, CudaExtent.width * sizeof(unsigned short), CudaExtent.width, CudaExtent.height);
		CopyParams.dstArray		= pCudaArray;
		CopyParams.extent		= CudaExtent;
		CopyParams.kind			= cudaMemcpyHostToDevice;
		
		HandleCudaError(cudaMemcpy3D(&CopyParams));

		TextureReference.normalized		= Normalized;
		TextureReference.filterMode		= TextureFilterMode;      
		TextureReference.addressMode[0]	= TextureAddressMode;  
		TextureReference.addressMode[1]	= TextureAddressMode;
  		TextureReference.addressMode[2]	= TextureAddressMode;

		HandleCudaError(cudaBindTextureToArray(&TextureReference, pCudaArray, &ChannelDescription));

		CUDA::ThreadSynchronize();
	}

	static void GetSymbolAddress(void** pDevicePointer, char* pSymbol)
	{
		CUDA::ThreadSynchronize();
		HandleCudaError(cudaGetSymbolAddress(pDevicePointer, pSymbol), "cudaGetSymbolAddress");
	}
};

template<typename T, int MaxSize = 256>
struct CudaList
{
	map<int, T>							ResourceMap;
	typename map<int, T>::iterator		ResourceMapIt;
	map<int, int>						HashMap;
	typename map<int, int>::iterator	HashMapIt;
	int									ResourceCounter;
	char								DeviceSymbol[MAX_CHAR_SIZE];
	T*									DevicePtr;
	
	HOST CudaList(const char* pDeviceSymbol)
	{
		this->ResourceCounter = 0;

		sprintf_s(DeviceSymbol, MAX_CHAR_SIZE, "%s", pDeviceSymbol);

		this->DevicePtr = NULL;
	}

	HOST ~CudaList()
	{
//		CUDA::Free(this->DevicePtr);
	}
	
	HOST bool Exists(int ID)
	{
		if (ID < 0)
			return false;

		this->ResourceMapIt = this->ResourceMap.find(ID);

		return this->ResourceMapIt != this->ResourceMap.end();
	}

	HOST void Bind(const T& Resource, int& ID)
	{
		if (this->ResourceMap.size() >= MaxSize)
			throw(Exception(Enums::Warning, "Maximum number of ResourceMap reached"));

		const bool Exists = this->Exists(ID);
		
		if (!Exists)
		{
			ID = this->ResourceCounter;
			this->ResourceMap[ID].FromHost(Resource);
			this->ResourceCounter++;
		}
		else
		{
			this->ResourceMap[ID].FromHost(Resource);
		}

		this->Synchronize();
	}

	HOST void Unbind(int ID)
	{
		if (!this->Exists(ID))
			return;

		this->ResourceMapIt = this->ResourceMap.find(ID);

		if (this->ResourceMapIt != this->ResourceMap.end())
			this->ResourceMap.erase(this->ResourceMapIt);

		this->HashMapIt = this->HashMap.find(ID);

		if (this->HashMapIt != this->HashMap.end())
			this->HashMap.erase(this->HashMapIt);

		this->Synchronize();
	}

	HOST void Synchronize()
	{
		if (this->ResourceMap.size() <= 0)
			return;

		T* pHostList = new T[this->ResourceMap.size()];
	
		int Size = 0;

		for (this->ResourceMapIt = this->ResourceMap.begin(); this->ResourceMapIt != this->ResourceMap.end(); this->ResourceMapIt++)
		{
			pHostList[Size] = this->ResourceMapIt->second;
			HashMap[this->ResourceMapIt->first] = Size;
			Size++;
		}
		
		CUDA::Free(this->DevicePtr);
		CUDA::Allocate(this->DevicePtr, (int)this->ResourceMap.size());
		CUDA::MemCopyHostToDevice(pHostList, this->DevicePtr, Size);
		CUDA::MemCopyHostToDeviceSymbol(&this->DevicePtr, this->DeviceSymbol);

		delete[] pHostList;
	}

	HOST T& operator[](const int& i)
	{
		this->ResourceMapIt = this->ResourceMap.find(i);

		if (this->ResourceMapIt == ResourceMap.end())
			throw(ErException(Enums::Fatal, "Resource does not exist"));

		return this->ResourceMap[i];
	}
};

#define LAUNCH_DIMENSIONS(width, height, depth, block_width, block_height, block_depth)						\
																											\
	dim3 BlockDim;																							\
																											\
	BlockDim.x = block_width;																				\
	BlockDim.y = block_height;																				\
	BlockDim.z = block_depth;																				\
																											\
	dim3 GridDim;																							\
																											\
	GridDim.x = (int)ceilf((float)width / (float)BlockDim.x);												\
	GridDim.y = (int)ceilf((float)height / (float)BlockDim.y);												\
	GridDim.z = (int)ceilf((float)depth / (float)BlockDim.z);

#define LAUNCH_CUDA_KERNEL_TIMED(cudakernelcall, title)														\
{																											\
	cudaEvent_t EventStart, EventStop;																		\
																											\
	CUDA::HandleCudaError(cudaEventCreate(&EventStart));													\
	CUDA::HandleCudaError(cudaEventCreate(&EventStop));														\
	CUDA::HandleCudaError(cudaEventRecord(EventStart, 0));													\
																											\
	cudakernelcall;																							\
																											\
	CUDA::HandleCudaError(cudaGetLastError());																\
	CUDA::HandleCudaError(cudaThreadSynchronize());															\
																											\
	CUDA::HandleCudaError(cudaEventRecord(EventStop, 0));													\
	CUDA::HandleCudaError(cudaEventSynchronize(EventStop));													\
																											\
	float TimeDelta = 0.0f;																					\
																											\
	CUDA::HandleCudaError(cudaEventElapsedTime(&TimeDelta, EventStart, EventStop), title);					\
																											\
	/*gKernelTimings.Add(ErKernelTiming(title, TimeDelta));*/												\
																											\
	CUDA::HandleCudaError(cudaEventDestroy(EventStart));													\
	CUDA::HandleCudaError(cudaEventDestroy(EventStop));														\
}

#define LAUNCH_CUDA_KERNEL(cudakernelcall)																	\
{																											\
	cudakernelcall;																							\
																											\
	CUDA::HandleCudaError(cudaGetLastError());																\
	CUDA::HandleCudaError(cudaThreadSynchronize());															\
}

#define KERNEL_1D(width)																					\
	const int IDx 	= blockIdx.x * blockDim.x + threadIdx.x;												\
	const int IDt	= threadIdx.x;																			\
	const int IDk	= IDx;																					\
																											\
	if (IDx >= width)																						\
		return;

#define KERNEL_2D(width, height)																			\
	const int IDx 	= blockIdx.x * blockDim.x + threadIdx.x;												\
	const int IDy 	= blockIdx.y * blockDim.y + threadIdx.y;												\
	const int IDt	= threadIdx.y * blockDim.x + threadIdx.x;												\
	const int IDk	= IDy * width + IDx;																	\
																											\
	if (IDx >= width || IDy >= height)																		\
		return;

#define KERNEL_3D(width, height, depth)																		\
	const int IDx 	= blockIdx.x * blockDim.x + threadIdx.x;												\
	const int IDy 	= blockIdx.y * blockDim.y + threadIdx.y;												\
	const int IDz 	= blockIdx.z * blockDim.z + threadIdx.z;												\
	const int IDt	= threadIdx.z * blockDim.x * blockDim.y + threadIdx.y * blockDim.x + threadIdx.x;		\
	const int IDk	= IDz * width * height + IDy * width + IDx;												\
																											\
	if (IDx >= width || IDy >= height || IDz >= depth)														\
		return;


}