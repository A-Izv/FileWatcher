#ifndef WIDGET_H
#define WIDGET_H
//------------------------------------------------------------------------------
#include <QWidget>
//------------------------------------------------------------------------------
#include <QTime>
#include <QDebug>
#include <QSettings>
#include <QFileDialog>
#include <QFileSystemWatcher>
//------------------------------------------------------------------------------
#include <QIcon>
#include <QMenu>
#include <QSystemTrayIcon>
//------------------------------------------------------------------------------
namespace Ui {
class mainWDG;
}
class WorkThread;
//------------------------------------------------------------------------------
class mainWDG : public QWidget
{
    Q_OBJECT

public:
    explicit            mainWDG(QWidget *parent = 0);
                       ~mainWDG();

private slots:
    void                on_dirNamePB_clicked();

    void                on_dirChange( const QString& path );

    void                trayClicks( QSystemTrayIcon::ActivationReason r );

    // теперь LOG - это слот
    void                log( const QString &s, const QString &title = QString() );
private:
    Ui::mainWDG         *ui;

    QSettings           settings;
    QString             dirName;

    QFileSystemWatcher  fsw;
    QStringList         fileList;

    QMenu               *trayMenu;
    QSystemTrayIcon     *trayIcon;

    WorkThread          *workThread;

    void                hideEvent( QHideEvent * );

    void                startWatching();
    void                stopWatching();

    void                process( const QString &name );
};
//------------------------------------------------------------------------------
#endif // WIDGET_H
