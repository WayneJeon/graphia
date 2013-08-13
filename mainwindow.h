#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void on_actionOpen_triggered();
    void on_actionQuit_triggered();
    void on_tabs_tabCloseRequested(int index);

    void on_loadProgress(int percentage);
    void on_loadCompletion(int success);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
