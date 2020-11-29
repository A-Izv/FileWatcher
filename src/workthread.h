#ifndef WORKTHREAD_H
#define WORKTHREAD_H
//------------------------------------------------------------------------------
#include <QThread>
//------------------------------------------------------------------------------
class QMutex;
//------------------------------------------------------------------------------
/**/
#include <ippCustom.h>
/**/ class SimpleIppFIR2File;
//------------------------------------------------------------------------------
class WorkThread : public QThread
{
    Q_OBJECT

public:
                                    WorkThread();
                                   ~WorkThread();

/**/// Метод добавления нового задания.
/**///   fileName - имя исходного файла (должен существовать, может быть пока не дописан)
/**///   outPath  - каталог для сохранения результата (если не существует - будет создан)
/**///   frqs     - частоты в диапазоне  [-0.5 , 0.5)
/**/    void                        addTask( const QString      &fileName,
/**/                                         const QString      &outPath,
/**/                                         const QList<float> &frqs      );
signals:
    void                            outMessage( const QString &m );
private:
/**/    // структура для хранения параметров заданий
/**/    struct Task
/**/    {
/**/        QString         fileName;
/**/        QString         outPath;
/**/        QList<float>    frqs;
/**/    };
/**/    static const int            BUFFER_SIZE_SMPL = 1<<20;
/**/    static const int            BUFFER_SIZE_BYTE = BUFFER_SIZE_SMPL*sizeof(Ipp32fc);
private:
    // результат обработки задания
    enum ProcessingResult {
        prNotDoneYet,   // задание еще не выполнено
        prDone,         // выполнено!
        prImposible,    // задание выполнить невозможно (будет удалено из очереди)
        prError         // ошибка при выполнении задания
    };
private:
/**/    Ipp32fc                     *buf;
/**/    QList<SimpleIppFIR2File*>   filters;
/**/
/**/    void                        initFilters( const QString      &outPath,
/**/                                             const QString      &srcFileName,
/**/                                             const QList<float> &frqs );
/**/    void                        filterPortion( Ipp32fc *src, int len );
/**/    void                        clearFilters();
private:
    bool                            errorFlag;

    QMutex                          *taskMutex;
    QList<Task>                     taskQueue;
    QList<Task>                     workQueue;

    void                            moveTasksToWorkQueue();
    bool                            shouldSleepForAWhile();

    void                            processWorkQueue();
    ProcessingResult                processTask( const Task &t );

    // QThread interface
protected:
    void                            run();
};
//------------------------------------------------------------------------------
#endif // WORKTHREAD_H
