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

#ifndef __ctkVTKOpenVRAbstractView_p_h
#define __ctkVTKOpenVRAbstractView_p_h

// Qt includes
#include <QObject>
#include <QTime>
class QTimer;

// CTK includes
#include "ctkVTKOpenVRAbstractView.h"

// VTK includes
#include <QVTKWidget.h>
#include <vtkCornerAnnotation.h>
#include <vtkOpenVRRenderWindow.h>
#include <vtkOpenVRRenderer.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>

//-----------------------------------------------------------------------------
/// \ingroup Visualization_VTK_Widgets
class ctkVTKOpenVRAbstractViewPrivate : public QObject
{
  Q_OBJECT
  Q_DECLARE_PUBLIC(ctkVTKOpenVRAbstractView);

protected:
  ctkVTKOpenVRAbstractView* const q_ptr;

public:
  ctkVTKOpenVRAbstractViewPrivate(ctkVTKOpenVRAbstractView& object);

  /// Convenient setup methods
  virtual void init();
  virtual void setupCornerAnnotation();
  virtual void setupRendering();

  QList<vtkRenderer*> renderers()const;
  vtkOpenVRRenderer* firstRenderer()const;

  QVTKWidget*                                   VTKWidget;
  vtkSmartPointer<vtkOpenVRRenderWindow>              RenderWindow;
  QTimer*                                       RequestTimer;
  QTime                                         RequestTime;
  bool                                          RenderEnabled;
  bool                                          FPSVisible;
  QTimer*                                       FPSTimer;
  int                                           FPS;
  static int                                    MultiSamples;

  vtkSmartPointer<vtkCornerAnnotation>          CornerAnnotation;
};

#endif
