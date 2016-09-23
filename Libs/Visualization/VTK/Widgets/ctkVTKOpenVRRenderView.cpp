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

// Qt includes
#include <QTimer>
#include <QVBoxLayout>
#include <QDebug>

// CTK includes
#include "ctkVTKOpenVRRenderView.h"
#include "ctkVTKOpenVRRenderView_p.h"

// VTK includes
#include <vtkRendererCollection.h>
#include <vtkOpenVRRenderWindowInteractor.h>
#include <vtkTextProperty.h>
#include <vtkCamera.h>

// --------------------------------------------------------------------------
// ctkVTKRenderViewPrivate methods

// --------------------------------------------------------------------------
ctkVTKOpenVRRenderViewPrivate::ctkVTKOpenVRRenderViewPrivate(ctkVTKOpenVRRenderView& object)
  :ctkVTKOpenVRAbstractViewPrivate(object)
{
  qRegisterMetaType<ctkAxesWidget::Axis>("ctkAxesWidget::Axis");
  this->Renderer = vtkSmartPointer<vtkOpenVRRenderer>::New();
  this->Axes = vtkSmartPointer<vtkAxesActor>::New();
  this->Orientation = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
  this->ZoomFactor = 0.05;
  this->PitchRollYawIncrement = 5;
  this->PitchDirection = ctkVTKOpenVRRenderView::PitchUp;
  this->RollDirection = ctkVTKOpenVRRenderView::RollRight;
  this->YawDirection = ctkVTKOpenVRRenderView::YawLeft;
  this->SpinDirection = ctkVTKOpenVRRenderView::YawRight;
  this->SpinEnabled = false;
  this->AnimationIntervalMs = 5;
  this->SpinIncrement = 2;
  this->RockEnabled = false;
  this->RockIncrement = 0;
  this->RockLength = 200;

  this->Orientation->SetOrientationMarker(this->Axes);
}

// --------------------------------------------------------------------------
void ctkVTKOpenVRRenderViewPrivate::setupCornerAnnotation()
{
  this->ctkVTKOpenVRAbstractViewPrivate::setupCornerAnnotation();
  if (!this->Renderer->HasViewProp(this->CornerAnnotation))
    {
    this->Renderer->AddViewProp(this->CornerAnnotation);
    }
}

//---------------------------------------------------------------------------
void ctkVTKOpenVRRenderViewPrivate::setupRendering()
{
  // Add renderer
  this->RenderWindow->AddRenderer(this->Renderer);
  this->ctkVTKOpenVRAbstractViewPrivate::setupRendering();
}

//----------------------------------------------------------------------------
void ctkVTKOpenVRRenderViewPrivate::zoom(double zoomFactor)
{
  Q_ASSERT(this->Renderer->IsActiveCameraCreated());
  vtkCamera * camera = this->Renderer->GetActiveCamera();

  if (camera->GetParallelProjection())
    {
    camera->SetParallelScale(camera->GetParallelScale() / (1 + zoomFactor));
    }
  else
    {
    camera->Dolly(1 + zoomFactor);
    this->Renderer->ResetCameraClippingRange();
    this->Renderer->UpdateLightsGeometryToFollowCamera();
    }
}

//---------------------------------------------------------------------------
void ctkVTKOpenVRRenderViewPrivate::pitch(double rotateDegrees,
                                    ctkVTKOpenVRRenderView::RotateDirection pitchDirection)
{
  Q_ASSERT(this->Renderer->IsActiveCameraCreated());
  Q_ASSERT(rotateDegrees >= 0);
  vtkCamera *cam = this->Renderer->GetActiveCamera();
  cam->Elevation(pitchDirection == ctkVTKOpenVRRenderView::PitchDown ? rotateDegrees : -rotateDegrees);
  cam->OrthogonalizeViewUp();
  this->Renderer->UpdateLightsGeometryToFollowCamera();
}

//---------------------------------------------------------------------------
void ctkVTKOpenVRRenderViewPrivate::roll(double rotateDegrees,
                                    ctkVTKOpenVRRenderView::RotateDirection rollDirection)
{
  Q_ASSERT(this->Renderer->IsActiveCameraCreated());
  Q_ASSERT(rotateDegrees >= 0);

  vtkCamera *cam = this->Renderer->GetActiveCamera();
  cam->Roll(rollDirection == ctkVTKOpenVRRenderView::RollLeft ? rotateDegrees : -rotateDegrees);
  cam->OrthogonalizeViewUp();
  this->Renderer->UpdateLightsGeometryToFollowCamera();
}

//---------------------------------------------------------------------------
void ctkVTKOpenVRRenderViewPrivate::yaw(double rotateDegrees,
                                    ctkVTKOpenVRRenderView::RotateDirection yawDirection)
{
  Q_ASSERT(this->Renderer->IsActiveCameraCreated());
  Q_ASSERT(rotateDegrees >= 0);
  vtkCamera *cam = this->Renderer->GetActiveCamera();
  cam->Azimuth(yawDirection == ctkVTKOpenVRRenderView::YawLeft ? rotateDegrees : -rotateDegrees);
  cam->OrthogonalizeViewUp();
  this->Renderer->UpdateLightsGeometryToFollowCamera();
}

//---------------------------------------------------------------------------
void ctkVTKOpenVRRenderViewPrivate::doSpin()
{
  Q_Q(ctkVTKOpenVRRenderView);
  if (!this->SpinEnabled)
    {
    return;
    }

  switch (this->SpinDirection)
    {
    case ctkVTKOpenVRRenderView::PitchUp:
    case ctkVTKOpenVRRenderView::PitchDown:
      this->pitch(this->SpinIncrement, this->SpinDirection);
      break;
    case ctkVTKOpenVRRenderView::RollLeft:
    case ctkVTKOpenVRRenderView::RollRight:
      this->roll(this->SpinIncrement, this->SpinDirection);
      break;
    case ctkVTKOpenVRRenderView::YawLeft:
    case ctkVTKOpenVRRenderView::YawRight:
      this->yaw(this->SpinIncrement, this->SpinDirection);
      break;
    }

  q->forceRender();
  QTimer::singleShot(this->AnimationIntervalMs, this, SLOT(doSpin()));
}

//---------------------------------------------------------------------------
void ctkVTKOpenVRRenderViewPrivate::doRock()
{
  Q_Q(ctkVTKOpenVRRenderView);
  Q_ASSERT(this->Renderer->IsActiveCameraCreated());

  if (!this->RockEnabled)
    {
    return;
    }

  vtkCamera *camera = this->Renderer->GetActiveCamera();

  double frac = static_cast<double>(this->RockIncrement) / static_cast<double>(this->RockLength);
  double az = 1.5 * cos (2.0 * 3.1415926 * (frac - floor(frac)));
  this->RockIncrement++;
  this->RockIncrement = this->RockIncrement % this->RockLength;

  // Move the camera
  camera->Azimuth(az);
  camera->OrthogonalizeViewUp();

  //Make the lighting follow the camera to avoid illumination changes
  this->Renderer->UpdateLightsGeometryToFollowCamera();

  q->forceRender();
  QTimer::singleShot(this->AnimationIntervalMs, this, SLOT(doRock()));
}

//---------------------------------------------------------------------------
// ctkVTKRenderView methods

// --------------------------------------------------------------------------
ctkVTKOpenVRRenderView::ctkVTKOpenVRRenderView(QWidget* parentWidget)
  : Superclass(new ctkVTKOpenVRRenderViewPrivate(*this), parentWidget)
{
  Q_D(ctkVTKOpenVRRenderView);
  d->init();

  // The interactor in RenderWindow exists after the renderwindow is set to
  // the QVTKWidet
  d->Orientation->SetInteractor(d->RenderWindow->GetInteractor());
  d->Orientation->SetEnabled(1);
  d->Orientation->InteractiveOff();
}

//----------------------------------------------------------------------------
ctkVTKOpenVRRenderView::~ctkVTKOpenVRRenderView()
{
}
//----------------------------------------------------------------------------
void ctkVTKOpenVRRenderView::setInteractor(vtkOpenVRRenderWindowInteractor* newInteractor)
{
  Q_D(ctkVTKOpenVRRenderView);
  this->Superclass::setInteractor(newInteractor);
  d->Orientation->SetInteractor(newInteractor);
}

//----------------------------------------------------------------------------
void ctkVTKOpenVRRenderView::setOrientationWidgetVisible(bool visible)
{
  Q_D(ctkVTKOpenVRRenderView);
  d->Orientation->SetEnabled(visible);
}

//----------------------------------------------------------------------------
bool ctkVTKOpenVRRenderView::orientationWidgetVisible()
{
  Q_D(ctkVTKOpenVRRenderView);
  return d->Orientation->GetEnabled();
}

//----------------------------------------------------------------------------
vtkCamera* ctkVTKOpenVRRenderView::activeCamera()
{
  Q_D(ctkVTKOpenVRRenderView);
  if (d->Renderer->IsActiveCameraCreated())
    {
    return d->Renderer->GetActiveCamera();
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
void ctkVTKOpenVRRenderView::resetCamera()
{
  Q_D(ctkVTKOpenVRRenderView);
  d->Renderer->ResetCamera();
}

//----------------------------------------------------------------------------
CTK_GET_CPP(ctkVTKOpenVRRenderView, vtkOpenVRRenderer*, renderer, Renderer);

//----------------------------------------------------------------------------
CTK_GET_CPP(ctkVTKOpenVRRenderView, double, pitchRollYawIncrement, PitchRollYawIncrement);

//----------------------------------------------------------------------------
void ctkVTKOpenVRRenderView::setPitchRollYawIncrement(double newPitchRollYawIncrement)
{
  Q_D(ctkVTKOpenVRRenderView);
  d->PitchRollYawIncrement = qAbs(newPitchRollYawIncrement);
}

//----------------------------------------------------------------------------
CTK_GET_CPP(ctkVTKOpenVRRenderView, ctkVTKOpenVRRenderView::RotateDirection, pitchDirection, PitchDirection);

//----------------------------------------------------------------------------
void ctkVTKOpenVRRenderView::setPitchDirection(ctkVTKOpenVRRenderView::RotateDirection newPitchDirection)
{
  Q_D(ctkVTKOpenVRRenderView);
  if (newPitchDirection != ctkVTKOpenVRRenderView::PitchUp &&
      newPitchDirection != ctkVTKOpenVRRenderView::PitchDown)
    {
    return;
    }
  d->PitchDirection = newPitchDirection;
}

//----------------------------------------------------------------------------
CTK_GET_CPP(ctkVTKOpenVRRenderView, ctkVTKOpenVRRenderView::RotateDirection, rollDirection, RollDirection);

//----------------------------------------------------------------------------
void ctkVTKOpenVRRenderView::setRollDirection(ctkVTKOpenVRRenderView::RotateDirection newRollDirection)
{
  Q_D(ctkVTKOpenVRRenderView);
  if (newRollDirection != ctkVTKOpenVRRenderView::RollLeft &&
      newRollDirection != ctkVTKOpenVRRenderView::RollRight)
    {
    return;
    }
  d->RollDirection = newRollDirection;
}

//----------------------------------------------------------------------------
CTK_GET_CPP(ctkVTKOpenVRRenderView, ctkVTKOpenVRRenderView::RotateDirection, yawDirection, YawDirection);

//----------------------------------------------------------------------------
void ctkVTKOpenVRRenderView::setYawDirection(ctkVTKOpenVRRenderView::RotateDirection newYawDirection)
{
  Q_D(ctkVTKOpenVRRenderView);
  if (newYawDirection != ctkVTKOpenVRRenderView::YawLeft &&
      newYawDirection != ctkVTKOpenVRRenderView::YawRight)
    {
    return;
    }
  d->YawDirection = newYawDirection;
}

//----------------------------------------------------------------------------
CTK_GET_CPP(ctkVTKOpenVRRenderView, ctkVTKOpenVRRenderView::RotateDirection, spinDirection, SpinDirection);
CTK_SET_CPP(ctkVTKOpenVRRenderView, ctkVTKOpenVRRenderView::RotateDirection, setSpinDirection, SpinDirection);

//----------------------------------------------------------------------------
void ctkVTKOpenVRRenderView::pitch()
{
  Q_D(ctkVTKOpenVRRenderView);
  if (!d->Renderer->IsActiveCameraCreated())
    {
    return;
    }
  d->pitch(d->PitchRollYawIncrement, d->PitchDirection);
}

//----------------------------------------------------------------------------
void ctkVTKOpenVRRenderView::roll()
{
  Q_D(ctkVTKOpenVRRenderView);
  if (!d->Renderer->IsActiveCameraCreated())
    {
    return;
    }
  d->roll(d->PitchRollYawIncrement, d->RollDirection);
}

//----------------------------------------------------------------------------
void ctkVTKOpenVRRenderView::yaw()
{
  Q_D(ctkVTKOpenVRRenderView);
  if (!d->Renderer->IsActiveCameraCreated())
    {
    return;
    }
  d->yaw(d->PitchRollYawIncrement, d->YawDirection);
}

//----------------------------------------------------------------------------
void ctkVTKOpenVRRenderView::setSpinEnabled(bool enabled)
{
  Q_D(ctkVTKOpenVRRenderView);
  if (enabled == d->SpinEnabled)
    {
    return;
    }
  d->SpinEnabled = enabled;
  d->RockEnabled = false;

  QTimer::singleShot(0, d, SLOT(doSpin()));
}

//----------------------------------------------------------------------------
CTK_GET_CPP(ctkVTKOpenVRRenderView, bool, spinEnabled, SpinEnabled);

//----------------------------------------------------------------------------
void ctkVTKOpenVRRenderView::setSpinIncrement(double newSpinIncrement)
{
  Q_D(ctkVTKOpenVRRenderView);
  d->SpinIncrement = qAbs(newSpinIncrement);
}

//----------------------------------------------------------------------------
CTK_GET_CPP(ctkVTKOpenVRRenderView, double, spinIncrement, SpinIncrement);

//----------------------------------------------------------------------------
void ctkVTKOpenVRRenderView::setAnimationIntervalMs(int newAnimationIntervalMs)
{
  Q_D(ctkVTKOpenVRRenderView);
  d->AnimationIntervalMs = qAbs(newAnimationIntervalMs);
}

//----------------------------------------------------------------------------
CTK_GET_CPP(ctkVTKOpenVRRenderView, int, animationIntervalMs, AnimationIntervalMs);

//----------------------------------------------------------------------------
void ctkVTKOpenVRRenderView::setRockEnabled(bool enabled)
{
  Q_D(ctkVTKOpenVRRenderView);
  if (enabled == d->RockEnabled)
    {
    return;
    }
  d->RockEnabled = enabled;
  d->SpinEnabled = false;

  QTimer::singleShot(0, d, SLOT(doRock()));
}

//----------------------------------------------------------------------------
CTK_GET_CPP(ctkVTKOpenVRRenderView, bool, rockEnabled, RockEnabled);

//----------------------------------------------------------------------------
void ctkVTKOpenVRRenderView::setRockLength(int newRockLength)
{
  Q_D(ctkVTKOpenVRRenderView);
  d->RockLength = qAbs(newRockLength);
}

//----------------------------------------------------------------------------
CTK_GET_CPP(ctkVTKOpenVRRenderView, int, rockLength, RockLength);

//----------------------------------------------------------------------------
void ctkVTKOpenVRRenderView::setRockIncrement(int newRockIncrement)
{
  Q_D(ctkVTKOpenVRRenderView);
  d->RockIncrement = qAbs(newRockIncrement);
}

//----------------------------------------------------------------------------
CTK_GET_CPP(ctkVTKOpenVRRenderView, int, rockIncrement, RockIncrement);

//----------------------------------------------------------------------------
void ctkVTKOpenVRRenderView::setZoomFactor(double newZoomFactor)
{
  Q_D(ctkVTKOpenVRRenderView);
  d->ZoomFactor = qBound(0.0, qAbs(newZoomFactor), 1.0);
}

//----------------------------------------------------------------------------
CTK_GET_CPP(ctkVTKOpenVRRenderView, double, zoomFactor, ZoomFactor);

//----------------------------------------------------------------------------
void ctkVTKOpenVRRenderView::zoomIn()
{
  Q_D(ctkVTKOpenVRRenderView);
  if (!d->Renderer->IsActiveCameraCreated())
    {
    return;
    }
  d->zoom(d->ZoomFactor);
}

//----------------------------------------------------------------------------
void ctkVTKOpenVRRenderView::zoomOut()
{
  Q_D(ctkVTKOpenVRRenderView);
  if (!d->Renderer->IsActiveCameraCreated())
    {
    return;
    }
  d->zoom(-d->ZoomFactor);
}

//----------------------------------------------------------------------------
void ctkVTKOpenVRRenderView::setFocalPoint(double x, double y, double z)
{
  Q_D(ctkVTKOpenVRRenderView);
  if (!d->Renderer->IsActiveCameraCreated())
    {
    return;
    }
  vtkCamera * camera = d->Renderer->GetActiveCamera();
  camera->SetFocalPoint(x, y, z);
  camera->ComputeViewPlaneNormal();
  camera->OrthogonalizeViewUp();
  d->Renderer->UpdateLightsGeometryToFollowCamera();
}

//----------------------------------------------------------------------------
void ctkVTKOpenVRRenderView::resetFocalPoint()
{
  Q_D(ctkVTKOpenVRRenderView);
  double bounds[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  d->Renderer->ComputeVisiblePropBounds(bounds);
  double x_center = (bounds[1] + bounds[0]) / 2.0;
  double y_center = (bounds[3] + bounds[2]) / 2.0;
  double z_center = (bounds[5] + bounds[4]) / 2.0;
  this->setFocalPoint(x_center, y_center, z_center);
}

//----------------------------------------------------------------------------
void ctkVTKOpenVRRenderView::lookFromAxis(const ctkAxesWidget::Axis& axis, double fov)
{
  Q_D(ctkVTKOpenVRRenderView);
  Q_ASSERT(d->Renderer);
  if (!d->Renderer->IsActiveCameraCreated())
    {
    return;
    }
  vtkCamera * camera = d->Renderer->GetActiveCamera();
  Q_ASSERT(camera);
  double widefov = fov*3;
  double* focalPoint = camera->GetFocalPoint();
  switch (axis)
    {
    case ctkAxesWidget::Right:
      camera->SetPosition(focalPoint[0]+widefov, focalPoint[1], focalPoint[2]);
      camera->SetViewUp(0, 0, 1);
      break;
    case ctkAxesWidget::Left:
      camera->SetPosition(focalPoint[0]-widefov, focalPoint[1], focalPoint[2]);
      camera->SetViewUp(0, 0, 1);
      break;
    case ctkAxesWidget::Anterior:
      camera->SetPosition(focalPoint[0], focalPoint[1]+widefov, focalPoint[2]);
      camera->SetViewUp(0, 0, 1);
      break;
    case ctkAxesWidget::Posterior:
      camera->SetPosition(focalPoint[0], focalPoint[1]-widefov, focalPoint[2]);
      camera->SetViewUp(0, 0, 1);
      break;
    case ctkAxesWidget::Superior:
      camera->SetPosition(focalPoint[0], focalPoint[1], focalPoint[2]+widefov);
      camera->SetViewUp(0, 1, 0);
      break;
    case ctkAxesWidget::Inferior:
      camera->SetPosition(focalPoint[0], focalPoint[1], focalPoint[2]-widefov);
      camera->SetViewUp(0, 1, 0);
      break;
    case ctkAxesWidget::None:
    default:
      // do nothing
      return;
      break;
    }
  d->Renderer->ResetCameraClippingRange();
  camera->ComputeViewPlaneNormal();
  camera->OrthogonalizeViewUp();
  d->Renderer->UpdateLightsGeometryToFollowCamera();
}
