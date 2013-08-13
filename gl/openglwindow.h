#ifndef OPENGLWINDOW_H
#define OPENGLWINDOW_H

#include <QWindow>
#include <QTime>

class CameraScene;
class AbstractScene;
class CameraController;
class QOpenGLDebugMessage;

class OpenGLWindow : public QWindow
{
    Q_OBJECT

public:
    explicit OpenGLWindow( const QSurfaceFormat& format,
                           QScreen* parent = 0 );
    //FIXME destructor to delete things

    QOpenGLContext* context() const { return m_context; }

    void setScene( AbstractScene* scene );
    AbstractScene* scene() const { return m_scene; }
    
protected:
    virtual void initialise();
    virtual void resize();
    virtual void render();

    virtual void keyReleaseEvent( QKeyEvent* e );
    virtual void mousePressEvent( QMouseEvent* e );
    virtual void mouseReleaseEvent( QMouseEvent* e );
    virtual void mouseMoveEvent( QMouseEvent* e );

protected slots:
    virtual void updateScene();

    void resizeEvent( QResizeEvent* e );
    void keyPressEvent( QKeyEvent* e );

    void messageLogged(const QOpenGLDebugMessage &message);

private:
    QOpenGLContext* m_context;
    AbstractScene* m_scene;
    CameraController* m_controller;
    
    QTime m_time;

    bool m_leftButtonPressed;
    QPoint m_prevPos;
    QPoint m_pos;
};

#endif // OPENGLWINDOW_H
