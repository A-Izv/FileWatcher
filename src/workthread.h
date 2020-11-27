#ifndef WORKTHREAD_H
#define WORKTHREAD_H
//------------------------------------------------------------------------------
#include <QThread>
#include <QMutex>
//------------------------------------------------------------------------------
class WorkThread : public QThread
{
    Q_OBJECT

public:
                            WorkThread();
                           ~WorkThread();

    void                    addTask( const QString &fileName );
signals:
    void                    outMessage( const QString &m );
private:
    // структура для хранения параметров заданий
    struct Task
    {
        QString     fileName;
    };
    // результат обработки задания
    enum ProcessingResult {
        prNotDoneYet,   // задание еще не выполнено
        prDone,         // выполнено!
        prImposible,    // задание выполнить невозможно (будет удалено из очереди)
        prError         // ошибка при выполнении задания
    };
private:
    bool                    errorFlag;

    QMutex                  taskMutex;
    QList<Task>             taskQueue;
    QList<Task>             workQueue;

    void                    moveTasksToWorkQueue();
    bool                    shouldSleepForAWhile();

    void                    processWorkQueue();
    ProcessingResult        processTask( const Task &t );

    // QThread interface
protected:
    void                    run();
};
//------------------------------------------------------------------------------
#endif // WORKTHREAD_H
