/*
	Copyright (c) 2011, T. Kroes <t.kroes@tudelft.nl>
	All rights reserved.

	Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

	- Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
	- Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
	- Neither the name of the TU Delft nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
	
	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ErCoreStable.h"

#include "vtkErVolumeInfo.h"
#include "vtkErVolumeProperty.h"

#include "Core.cuh"

#include <vtkBoundingBox.h>

vtkCxxRevisionMacro(vtkErVolumeInfo, "$Revision: 1.0 $");
vtkStandardNewMacro(vtkErVolumeInfo);

vtkErVolumeInfo::vtkErVolumeInfo() :
	m_VolumeInfo(),
	m_pIntensity(NULL),
	m_pGradientMagnitude(NULL),
	Volume(NULL)
{
}

vtkErVolumeInfo::~vtkErVolumeInfo()
{
}

void vtkErVolumeInfo::SetInputData(vtkImageData* pInputData)
{
	if (m_pIntensity != NULL)
		return;

    if (pInputData == NULL)
    {
//        this->CudaInputBuffer.Free();
    }
    else if (pInputData != m_pIntensity)
    {
		vtkSmartPointer<vtkImageCast> ImageCast = vtkImageCast::New();
		
		ImageCast->SetInput(pInputData);
		ImageCast->SetOutputScalarTypeToShort();
		ImageCast->Update();

		m_pIntensity = ImageCast->GetOutput();

		vtkSmartPointer<vtkImageGradientMagnitude> GradientMagnitude = vtkImageGradientMagnitude::New();

		GradientMagnitude->SetDimensionality(3);
		GradientMagnitude->SetInput(m_pIntensity);
		GradientMagnitude->Update();
		
		m_pGradientMagnitude = GradientMagnitude->GetOutput();
	
		int* pResolution = m_pIntensity->GetExtent();
		
		double* pBounds = m_pIntensity->GetBounds();

		m_VolumeInfo.m_Extent.x	= pBounds[1] - pBounds[0];
		m_VolumeInfo.m_Extent.y	= pBounds[3] - pBounds[2];
		m_VolumeInfo.m_Extent.z	= pBounds[5] - pBounds[4];
		
		m_VolumeInfo.m_InvExtent.x	= m_VolumeInfo.m_Extent.x != 0.0f ? 1.0f / m_VolumeInfo.m_Extent.x : 0.0f;
		m_VolumeInfo.m_InvExtent.y	= m_VolumeInfo.m_Extent.y != 0.0f ? 1.0f / m_VolumeInfo.m_Extent.y : 0.0f;
		m_VolumeInfo.m_InvExtent.z	= m_VolumeInfo.m_Extent.z != 0.0f ? 1.0f / m_VolumeInfo.m_Extent.z : 0.0f;

		cudaExtent Extent;

		Extent.width	= pResolution[1] + 1;
		Extent.height	= pResolution[3] + 1;
		Extent.depth	= pResolution[5] + 1;

		double* pIntensityRange = m_pIntensity->GetScalarRange();
		
		m_VolumeInfo.m_IntensityMin			= (float)pIntensityRange[0];
		m_VolumeInfo.m_IntensityMax			= (float)pIntensityRange[1];
		m_VolumeInfo.m_IntensityRange		= m_VolumeInfo.m_IntensityMax - m_VolumeInfo.m_IntensityMin;
		m_VolumeInfo.m_IntensityInvRange	= 1.0f / m_VolumeInfo.m_IntensityRange;

		double* pSpacing = m_pIntensity->GetSpacing();

		m_VolumeInfo.m_Spacing.x = (float)pSpacing[0];
		m_VolumeInfo.m_Spacing.y = (float)pSpacing[1];
		m_VolumeInfo.m_Spacing.z = (float)pSpacing[2];

		m_VolumeInfo.m_InvSpacing.x = m_VolumeInfo.m_Spacing.x != 0.0f ? 1.0f / m_VolumeInfo.m_Spacing.x : 0.0f;
		m_VolumeInfo.m_InvSpacing.y = m_VolumeInfo.m_Spacing.y != 0.0f ? 1.0f / m_VolumeInfo.m_Spacing.y : 0.0f;
		m_VolumeInfo.m_InvSpacing.z = m_VolumeInfo.m_Spacing.z != 0.0f ? 1.0f / m_VolumeInfo.m_Spacing.z : 0.0f;

		Vec3f PhysicalSize;

		PhysicalSize.x = m_VolumeInfo.m_Spacing.x * m_VolumeInfo.m_Extent.x;
		PhysicalSize.y = m_VolumeInfo.m_Spacing.y * m_VolumeInfo.m_Extent.y;
		PhysicalSize.z = m_VolumeInfo.m_Spacing.z * m_VolumeInfo.m_Extent.z;

		m_VolumeInfo.m_MinAABB.x	= pBounds[0];
		m_VolumeInfo.m_MinAABB.y	= pBounds[2];
		m_VolumeInfo.m_MinAABB.z	= pBounds[4];

		m_VolumeInfo.m_InvMinAABB.x	= m_VolumeInfo.m_MinAABB.x != 0.0f ? 1.0f / m_VolumeInfo.m_MinAABB.x : 0.0f;
		m_VolumeInfo.m_InvMinAABB.y	= m_VolumeInfo.m_MinAABB.y != 0.0f ? 1.0f / m_VolumeInfo.m_MinAABB.y : 0.0f;
		m_VolumeInfo.m_InvMinAABB.z	= m_VolumeInfo.m_MinAABB.z != 0.0f ? 1.0f / m_VolumeInfo.m_MinAABB.z : 0.0f;

		m_VolumeInfo.m_MaxAABB.x	= pBounds[1];
		m_VolumeInfo.m_MaxAABB.y	= pBounds[3];
		m_VolumeInfo.m_MaxAABB.z	= pBounds[5];

		m_VolumeInfo.m_InvMaxAABB.x	= m_VolumeInfo.m_MaxAABB.x != 0.0f ? 1.0f / m_VolumeInfo.m_MaxAABB.x : 0.0f;
		m_VolumeInfo.m_InvMaxAABB.y	= m_VolumeInfo.m_MaxAABB.y != 0.0f ? 1.0f / m_VolumeInfo.m_MaxAABB.y : 0.0f;
		m_VolumeInfo.m_InvMaxAABB.z	= m_VolumeInfo.m_MaxAABB.z != 0.0f ? 1.0f / m_VolumeInfo.m_MaxAABB.z : 0.0f;

		m_VolumeInfo.m_GradientDelta		= 1.0f;
		m_VolumeInfo.m_InvGradientDelta		= 1.0f;
		m_VolumeInfo.m_GradientDeltaX.x		= m_VolumeInfo.m_GradientDelta;
		m_VolumeInfo.m_GradientDeltaX.y		= 0.0f;
		m_VolumeInfo.m_GradientDeltaX.z		= 0.0f;
		m_VolumeInfo.m_GradientDeltaY.x		= 0.0f;
		m_VolumeInfo.m_GradientDeltaY.y		= m_VolumeInfo.m_GradientDelta;
		m_VolumeInfo.m_GradientDeltaY.z		= 0.0f;
		m_VolumeInfo.m_GradientDeltaZ.x		= 0.0f;
		m_VolumeInfo.m_GradientDeltaZ.y		= 0.0f;
		m_VolumeInfo.m_GradientDeltaZ.z		= m_VolumeInfo.m_GradientDelta;

		BindIntensityBuffer((short*)m_pGradientMagnitude->GetScalarPointer(), Extent);
		BindGradientMagnitudeBuffer((short*)m_pGradientMagnitude->GetScalarPointer(), Extent);
    }
}

void vtkErVolumeInfo::Update()
{
	vtkErVolumeProperty* pErVolumeProperty = dynamic_cast<vtkErVolumeProperty*>(GetVolume()->GetProperty());

	if (pErVolumeProperty)
	{
		m_VolumeInfo.m_DensityScale		= pErVolumeProperty->GetDensityScale();
		m_VolumeInfo.m_StepSize			= pErVolumeProperty->GetStepSizeFactorPrimary() * (m_VolumeInfo.m_MaxAABB.x / m_VolumeInfo.m_Extent.x);
		m_VolumeInfo.m_StepSizeShadow	= m_VolumeInfo.m_StepSize * pErVolumeProperty->GetStepSizeFactorSecondary();
		m_VolumeInfo.m_GradientDelta	= pErVolumeProperty->GetGradientDeltaFactor() * m_VolumeInfo.m_Spacing.x;
		m_VolumeInfo.m_InvGradientDelta	= 1.0f / m_VolumeInfo.m_GradientDelta;
		m_VolumeInfo.m_GradientFactor	= pErVolumeProperty->GetGradientFactor();
		m_VolumeInfo.m_ShadingType		= pErVolumeProperty->GetShadingType();
	}
	else
	{
		m_VolumeInfo.m_DensityScale		= vtkErVolumeProperty::DefaultDensityScale();
		m_VolumeInfo.m_StepSize			= vtkErVolumeProperty::DefaultStepSizeFactorPrimary() * (m_VolumeInfo.m_MaxAABB.x / m_VolumeInfo.m_Extent.x);
		m_VolumeInfo.m_StepSizeShadow	= vtkErVolumeProperty::DefaultStepSizeFactorSecondary() * m_VolumeInfo.m_StepSize;
		m_VolumeInfo.m_GradientDelta	= vtkErVolumeProperty::DefaultGradientDeltaFactor() * m_VolumeInfo.m_Spacing.x;
		m_VolumeInfo.m_InvGradientDelta	= 1.0f / m_VolumeInfo.m_GradientDelta;
		m_VolumeInfo.m_GradientFactor	= vtkErVolumeProperty::DefaultGradientFactor();
		m_VolumeInfo.m_ShadingType		= vtkErVolumeProperty::DefaultShadingType();
	}

	m_VolumeInfo.m_GradientDeltaX.x = m_VolumeInfo.m_GradientDelta;
	m_VolumeInfo.m_GradientDeltaX.y = 0.0f;
	m_VolumeInfo.m_GradientDeltaX.z = 0.0f;

	m_VolumeInfo.m_GradientDeltaY.x = 0.0f;
	m_VolumeInfo.m_GradientDeltaY.y = m_VolumeInfo.m_GradientDelta;
	m_VolumeInfo.m_GradientDeltaY.z = 0.0f;

	m_VolumeInfo.m_GradientDeltaZ.x = 0.0f;
	m_VolumeInfo.m_GradientDeltaZ.y = 0.0f;
	m_VolumeInfo.m_GradientDeltaZ.z = m_VolumeInfo.m_GradientDelta;
}