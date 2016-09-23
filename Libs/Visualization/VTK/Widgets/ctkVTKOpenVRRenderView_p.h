/*=========================================================================

  Library:   CTK

  Copyright (c) Kitware Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=========================================================================*/

#ifndef __ctkVTKOpenVRRenderView_p_h
#define __ctkVTKOpenVRRenderView_p_h

// Qt includes
#include <QObject>

// CTK includes
#include <ctkPimpl.h>
#include "ctkVTKOpenVRAbstractView_p.h"
#include <ctkVTKObject.h>
#include "ctkVTKOpenVRRenderView.h"

// VTK includes
#include <QVTKWidget.h>
#include <vtkAxesActor.h>
#include <vtkCornerAnnotation.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkOpenVRRenderer.h>
#include <vtkOpenVRRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>

class vtkRenderWindowInteractor;

//-----------------------------------------------------------------------------
/// \ingroup Visualization_VTK_Widgets
class ctkVTKOpenVRRenderViewPrivate : public ctkVTKOpenVRAbstractViewPrivate
{
  Q_OBJECT
  Q_DECLARE_PUBLIC(ctkVTKOpenVRRenderView);

public:
  ctkVTKOpenVRRenderViewPrivate(ctkVTKOpenVRRenderView& object);

  /// Convenient setup methods
  virtual void setupCornerAnnotation();
  virtual void setupRendering();

  void zoom(double zoomFactor);

  void pitch(double rotateDegrees, ctkVTKOpenVRRenderView::RotateDirection pitchDirection);
  void roll(double rotateDegrees, ctkVTKOpenVRRenderView::RotateDirection rollDirection);
  void yaw(double rotateDegrees, ctkVTKOpenVRRenderView::RotateDirection yawDirection);

public Q_SLOTS:
  void doSpin();
  void doRock();

public:

  vtkSmartPointer<vtkOpenVRRenderer>                  Renderer;

  vtkSmartPointer<vtkAxesActor>                 Axes;
  vtkSmartPointer<vtkOrientationMarkerWidget>   Orientation;

  double                                        ZoomFactor;
  double                                        PitchRollYawIncrement;
  ctkVTKOpenVRRenderView::RotateDirection             PitchDirection;
  ctkVTKOpenVRRenderView::RotateDirection             RollDirection;
  ctkVTKOpenVRRenderView::RotateDirection             YawDirection;
  ctkVTKOpenVRRenderView::RotateDirection             SpinDirection;
  bool                                          SpinEnabled;
  int                                           AnimationIntervalMs;
  double                                        SpinIncrement;
  bool                                          RockEnabled;
  int                                           RockIncrement;
  int                                           RockLength;
};

#endif
