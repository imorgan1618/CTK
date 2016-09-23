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
#include "ctkVTKOpenVRAbstractView.h"
#include "ctkVTKOpenVRAbstractView_p.h"
#include "ctkLogger.h"

// VTK includes
#include <vtkOpenGLRenderWindow.h>
#include <vtkRendererCollection.h>
//#include <vtkRenderWindowInteractor.h>
#include <vtkTextProperty.h>

// VTK OpenVR includes
#include <vtkOpenVRRenderWindowInteractor.h>
#include <vtkOpenVRRenderer.h>

//--------------------------------------------------------------------------
static ctkLogger logger("org.commontk.visualization.vtk.widgets.ctkVTKAbstractView");
//--------------------------------------------------------------------------
int ctkVTKOpenVRAbstractViewPrivate::MultiSamples = 0;  // Default for static var
//--------------------------------------------------------------------------

// --------------------------------------------------------------------------
// ctkVTKAbstractViewPrivate methods

// --------------------------------------------------------------------------
ctkVTKOpenVRAbstractViewPrivate::ctkVTKOpenVRAbstractViewPrivate(ctkVTKOpenVRAbstractView& object)
  : q_ptr(&object)
{
  this->RenderWindow = vtkSmartPointer<vtkOpenVRRenderWindow>::New();
  this->CornerAnnotation = vtkSmartPointer<vtkCornerAnnotation>::New();
  this->RequestTimer = 0;
  this->RenderEnabled = true;
  this->FPSVisible = false;
  this->FPSTimer = 0;
  this->FPS = 0;
}

// --------------------------------------------------------------------------
void ctkVTKOpenVRAbstractViewPrivate::init()
{
  Q_Q(ctkVTKOpenVRAbstractView);

  this->setParent(q);

  this->VTKWidget = new QVTKWidget;
  q->setLayout(new QVBoxLayout);
  q->layout()->setMargin(0);
  q->layout()->setSpacing(0);
  q->layout()->addWidget(this->VTKWidget);

  this->RequestTimer = new QTimer(q);
  this->RequestTimer->setSingleShot(true);
  QObject::connect(this->RequestTimer, SIGNAL(timeout()),
                   q, SLOT(forceRender()));

  this->FPSTimer = new QTimer(q);
  this->FPSTimer->setInterval(1000);
  QObject::connect(this->FPSTimer, SIGNAL(timeout()),
                   q, SLOT(updateFPS()));

  this->setupCornerAnnotation();
  this->setupRendering();

  // block renders and observe interactor to enforce framerate
  q->setInteractor(this->RenderWindow->GetInteractor());
}

// --------------------------------------------------------------------------
void ctkVTKOpenVRAbstractViewPrivate::setupCornerAnnotation()
{
  this->CornerAnnotation->SetMaximumLineHeight(0.07);
  vtkTextProperty *tprop = this->CornerAnnotation->GetTextProperty();
  tprop->ShadowOn();
  this->CornerAnnotation->ClearAllTexts();
}

//---------------------------------------------------------------------------
void ctkVTKOpenVRAbstractViewPrivate::setupRendering()
{
  Q_ASSERT(this->RenderWindow);
  this->RenderWindow->SetAlphaBitPlanes(1);
  int nSamples = ctkVTKOpenVRAbstractView::multiSamples();
  if (nSamples < 0)
    {
    nSamples = vtkOpenGLRenderWindow::GetGlobalMaximumNumberOfMultiSamples();
    }
  this->RenderWindow->SetMultiSamples(nSamples);
  this->RenderWindow->StereoCapableWindowOn();
  this->VTKWidget->SetRenderWindow(this->RenderWindow);
}

//---------------------------------------------------------------------------
QList<vtkRenderer*> ctkVTKOpenVRAbstractViewPrivate::renderers()const
{
  QList<vtkRenderer*> rendererList;

  vtkRendererCollection* rendererCollection = this->RenderWindow->GetRenderers();
  vtkCollectionSimpleIterator rendererIterator;
  rendererCollection->InitTraversal(rendererIterator);
  vtkRenderer *renderer;
  while ( (renderer= rendererCollection->GetNextRenderer(rendererIterator)) )
    {
    rendererList << renderer;
    }
  return rendererList;
}

//---------------------------------------------------------------------------
vtkOpenVRRenderer* ctkVTKOpenVRAbstractViewPrivate::firstRenderer()const
{
  return static_cast<vtkOpenVRRenderer*>(this->RenderWindow->GetRenderers()
    ->GetItemAsObject(0));
}

//---------------------------------------------------------------------------
// ctkVTKAbstractView methods

// --------------------------------------------------------------------------
ctkVTKOpenVRAbstractView::ctkVTKOpenVRAbstractView(QWidget* parentWidget)
  : Superclass(parentWidget)
  , d_ptr(new ctkVTKOpenVRAbstractViewPrivate(*this))
{
  Q_D(ctkVTKOpenVRAbstractView);
  d->init();
}

// --------------------------------------------------------------------------
ctkVTKOpenVRAbstractView::ctkVTKOpenVRAbstractView(ctkVTKOpenVRAbstractViewPrivate* pimpl, QWidget* parentWidget)
  : Superclass(parentWidget)
  , d_ptr(pimpl)
{
  // derived classes must call init manually. Calling init() here may results in
  // actions on a derived public class not yet finished to be created
}

//----------------------------------------------------------------------------
ctkVTKOpenVRAbstractView::~ctkVTKOpenVRAbstractView()
{
}

//----------------------------------------------------------------------------
void ctkVTKOpenVRAbstractView::scheduleRender()
{
  Q_D(ctkVTKOpenVRAbstractView);

  //logger.trace(QString("scheduleRender - RenderEnabled: %1 - Request render elapsed: %2ms").
  //             arg(d->RenderEnabled ? "true" : "false")
  //             .arg(d->RequestTime.elapsed()));

  if (!d->RenderEnabled)
    {
    return;
    }

  double msecsBeforeRender = 100. / d->RenderWindow->GetDesiredUpdateRate();
  if(d->VTKWidget->testAttribute(Qt::WA_WState_InPaintEvent))
    {
    // If the request comes from the system (widget exposed, resized...), the
    // render must be done immediately.
    this->forceRender();
    }
  else if (!d->RequestTime.isValid())
    {
    // If the DesiredUpdateRate is in "still mode", the requested framerate
    // is fake, it is just a way to allocate as much time as possible for the
    // rendering, it doesn't really mean that rendering must occur only once
    // every couple seconds. It just means it should be done when there is
    // time to do it. A timer of 0, kind of mean a rendering is done next time
    // it is idle.
    if (msecsBeforeRender > 10000)
      {
      msecsBeforeRender = 0;
      }
    d->RequestTime.start();
    d->RequestTimer->start(static_cast<int>(msecsBeforeRender));
    }
  else if (d->RequestTime.elapsed() > msecsBeforeRender)
    {
    // The rendering hasn't still be done, but msecsBeforeRender milliseconds
    // have already been elapsed, it is likely that RequestTimer has already
    // timed out, but the event queue hasn't been processed yet, rendering is
    // done now to ensure the desired framerate is respected.
    this->forceRender();
    }
}

//----------------------------------------------------------------------------
void ctkVTKOpenVRAbstractView::forceRender()
{
  Q_D(ctkVTKOpenVRAbstractView);

  if (this->sender() == d->RequestTimer  &&
      !d->RequestTime.isValid())
    {
    // The slot associated to the timeout signal is now called, however the
    // render has already been executed meanwhile. There is no need to do it
    // again.
    return;
    }

  // The timer can be stopped if it hasn't timed out yet.
  d->RequestTimer->stop();
  d->RequestTime = QTime();

  //logger.trace(QString("forceRender - RenderEnabled: %1")
  //             .arg(d->RenderEnabled ? "true" : "false"));

  if (!d->RenderEnabled || !this->isVisible())
    {
    return;
    }
  d->RenderWindow->Render();
}

//----------------------------------------------------------------------------
CTK_GET_CPP(ctkVTKOpenVRAbstractView, vtkOpenVRRenderWindow*, renderWindow, RenderWindow);

//----------------------------------------------------------------------------
void ctkVTKOpenVRAbstractView::setInteractor(vtkRenderWindowInteractor* newInteractor)
{
  Q_D(ctkVTKOpenVRAbstractView);

  d->RenderWindow->SetInteractor(newInteractor);
  // Prevent the interactor to call Render() on the render window; only
  // scheduleRender() and forceRender() can Render() the window.
  // This is done to ensure the desired framerate is respected.
  newInteractor->SetEnableRender(false);
  qvtkReconnect(d->RenderWindow->GetInteractor(), newInteractor,
                vtkCommand::RenderEvent, this, SLOT(scheduleRender()));
}

//----------------------------------------------------------------------------
vtkRenderWindowInteractor* ctkVTKOpenVRAbstractView::interactor()const
{
  Q_D(const ctkVTKOpenVRAbstractView);
  return d->RenderWindow->GetInteractor();
}

//----------------------------------------------------------------------------
vtkInteractorObserver* ctkVTKOpenVRAbstractView::interactorStyle()const
{
  return this->interactor() ?
    this->interactor()->GetInteractorStyle() : 0;
}

//----------------------------------------------------------------------------
void ctkVTKOpenVRAbstractView::setCornerAnnotationText(const QString& text)
{
  Q_D(ctkVTKOpenVRAbstractView);
  d->CornerAnnotation->ClearAllTexts();
  d->CornerAnnotation->SetText(2, text.toLatin1());
}

//----------------------------------------------------------------------------
QString ctkVTKOpenVRAbstractView::cornerAnnotationText() const
{
  Q_D(const ctkVTKOpenVRAbstractView);
  return QLatin1String(d->CornerAnnotation->GetText(2));
}

//----------------------------------------------------------------------------
vtkCornerAnnotation* ctkVTKOpenVRAbstractView::cornerAnnotation() const
{
  Q_D(const ctkVTKOpenVRAbstractView);
  return d->CornerAnnotation;
}

//----------------------------------------------------------------------------
QVTKWidget * ctkVTKOpenVRAbstractView::VTKWidget() const
{
  Q_D(const ctkVTKOpenVRAbstractView);
  return d->VTKWidget;
}

//----------------------------------------------------------------------------
CTK_SET_CPP(ctkVTKOpenVRAbstractView, bool, setRenderEnabled, RenderEnabled);
CTK_GET_CPP(ctkVTKOpenVRAbstractView, bool, renderEnabled, RenderEnabled);

//----------------------------------------------------------------------------
QSize ctkVTKOpenVRAbstractView::minimumSizeHint()const
{
  // Arbitrary size. 50x50 because smaller seems too small.
  return QSize(50, 50);
}

//----------------------------------------------------------------------------
QSize ctkVTKOpenVRAbstractView::sizeHint()const
{
  // Arbitrary size. 300x300 is the default vtkRenderWindow size.
  return QSize(300, 300);
}

//----------------------------------------------------------------------------
bool ctkVTKOpenVRAbstractView::hasHeightForWidth()const
{
  return true;
}

//----------------------------------------------------------------------------
int ctkVTKOpenVRAbstractView::heightForWidth(int width)const
{
  // typically VTK render window tend to be square...
  return width;
}

//----------------------------------------------------------------------------
void ctkVTKOpenVRAbstractView::setBackgroundColor(const QColor& newBackgroundColor)
{
  Q_D(ctkVTKOpenVRAbstractView);
  double color[3];
  color[0] = newBackgroundColor.redF();
  color[1] = newBackgroundColor.greenF();
  color[2] = newBackgroundColor.blueF();
  foreach(vtkRenderer* renderer, d->renderers())
    {
    renderer->SetBackground(color);
    }
}

//----------------------------------------------------------------------------
QColor ctkVTKOpenVRAbstractView::backgroundColor()const
{
  Q_D(const ctkVTKOpenVRAbstractView);
  vtkOpenVRRenderer* firstRenderer = d->firstRenderer();
  return firstRenderer ? QColor::fromRgbF(firstRenderer->GetBackground()[0],
                                          firstRenderer->GetBackground()[1],
                                          firstRenderer->GetBackground()[2])
                       : QColor();
}

//----------------------------------------------------------------------------
void ctkVTKOpenVRAbstractView::setBackgroundColor2(const QColor& newBackgroundColor)
{
  Q_D(ctkVTKOpenVRAbstractView);
  double color[3];
  color[0] = newBackgroundColor.redF();
  color[1] = newBackgroundColor.greenF();
  color[2] = newBackgroundColor.blueF();
  foreach(vtkRenderer* renderer, d->renderers())
    {
    renderer->SetBackground2(color);
    }
}

//----------------------------------------------------------------------------
QColor ctkVTKOpenVRAbstractView::backgroundColor2()const
{
  Q_D(const ctkVTKOpenVRAbstractView);
  vtkOpenVRRenderer* firstRenderer = d->firstRenderer();
  return firstRenderer ? QColor::fromRgbF(firstRenderer->GetBackground2()[0],
                                          firstRenderer->GetBackground2()[1],
                                          firstRenderer->GetBackground2()[2])
                       : QColor();
}

//----------------------------------------------------------------------------
void ctkVTKOpenVRAbstractView::setGradientBackground(bool enable)
{
  Q_D(ctkVTKOpenVRAbstractView);
  foreach(vtkRenderer* renderer, d->renderers())
    {
    renderer->SetGradientBackground(enable);
    }
}

//----------------------------------------------------------------------------
bool ctkVTKOpenVRAbstractView::gradientBackground()const
{
  Q_D(const ctkVTKOpenVRAbstractView);
  vtkOpenVRRenderer* firstRenderer = d->firstRenderer();
  return firstRenderer ? firstRenderer->GetGradientBackground() : false;
}

//----------------------------------------------------------------------------
void ctkVTKOpenVRAbstractView::setFPSVisible(bool show)
{
  Q_D(ctkVTKOpenVRAbstractView);
  if (d->FPSVisible == show)
    {
    return;
    }
  d->FPSVisible = show;
  vtkOpenVRRenderer* renderer = d->firstRenderer();
  if (d->FPSVisible)
    {
    d->FPSTimer->start();
    qvtkConnect(renderer,
                vtkCommand::EndEvent, this, SLOT(onRender()));
    }
  else
    {
    d->FPSTimer->stop();
    qvtkDisconnect(renderer,
                   vtkCommand::EndEvent, this, SLOT(onRender()));
    d->CornerAnnotation->SetText(1, "");
    }
}

//----------------------------------------------------------------------------
bool ctkVTKOpenVRAbstractView::isFPSVisible()const
{
  Q_D(const ctkVTKOpenVRAbstractView);
  return d->FPSVisible;
}

//----------------------------------------------------------------------------
void ctkVTKOpenVRAbstractView::onRender()
{
  Q_D(ctkVTKOpenVRAbstractView);
  ++d->FPS;
}

//----------------------------------------------------------------------------
void ctkVTKOpenVRAbstractView::updateFPS()
{
  Q_D(ctkVTKOpenVRAbstractView);
  vtkOpenVRRenderer* renderer = d->firstRenderer();
  double lastRenderTime = renderer ? renderer->GetLastRenderTimeInSeconds() : 0.;
  QString fpsString = tr("FPS: %1(%2s)").arg(d->FPS).arg(lastRenderTime);
  d->FPS = 0;
  d->CornerAnnotation->SetText(1, fpsString.toLatin1());
}

//----------------------------------------------------------------------------
bool ctkVTKOpenVRAbstractView::useDepthPeeling()const
{
  Q_D(const ctkVTKOpenVRAbstractView);
  vtkOpenVRRenderer* renderer = d->firstRenderer();
  return renderer ? static_cast<bool>(renderer->GetUseDepthPeeling()):0;
}

//----------------------------------------------------------------------------
void ctkVTKOpenVRAbstractView::setUseDepthPeeling(bool useDepthPeeling)
{
  Q_D(ctkVTKOpenVRAbstractView);
  vtkOpenVRRenderer* renderer = d->firstRenderer();
  if (!renderer)
    {
    return;
    }
  this->renderWindow()->SetAlphaBitPlanes( useDepthPeeling ? 1 : 0);
  int nSamples = ctkVTKOpenVRAbstractView::multiSamples();
  if (nSamples < 0)
    {
    nSamples = vtkOpenGLRenderWindow::GetGlobalMaximumNumberOfMultiSamples();
    }
  this->renderWindow()->SetMultiSamples(useDepthPeeling ? 0 : nSamples);
  renderer->SetUseDepthPeeling(useDepthPeeling ? 1 : 0);
}

//----------------------------------------------------------------------------
int ctkVTKOpenVRAbstractView::multiSamples()
{
  return ctkVTKOpenVRAbstractViewPrivate::MultiSamples;
}

//----------------------------------------------------------------------------
void ctkVTKOpenVRAbstractView::setMultiSamples(int number)
{
  ctkVTKOpenVRAbstractViewPrivate::MultiSamples = number;
}
