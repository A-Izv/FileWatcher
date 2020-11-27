#include "widget.h"
#include "ui_widget.h"
//------------------------------------------------------------------------------
mainWDG::mainWDG(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::mainWDG),
    settings( "theCompany", "FileWatcher" )
{
    ui->setupUi(this);

    log( "Программа запущена" );

 // объединяем сторожок со слотом
    QObject::connect( &fsw, SIGNAL(directoryChanged(QString)),
                      this, SLOT(on_dirChange(QString)) );
 // читаем настройки
    restoreGeometry( settings.value( "geometry" ).toByteArray() );
    dirName = settings.value( "dirName", "d:/test/" ).toString(); // !!! В UI НЕ ВЫВОДИТЬ
 // если настройки правильные - запускаем контроль
    if( QDir(dirName).exists() ) {
        startWatching();
    } else {
        log( "Прочитанная из настроек директория не существует" );
    }
 // выделяем память для буфера чтения
    buf = new int[BSIZE];
 // инициализируем таблицу разворота байт
    int preTable[16] = {
        0x0,
        0x8,
        0x4,
        0xC,
        0x2,
        0xA,
        0x6,
        0xE,
        0x1,
        0x9,
        0x5,
        0xD,
        0x3,
        0xB,
        0x7,
        0xF
    };
    for( int i = 0 ; i < 256 ; i++ ) {
        table[i]  =  preTable[ (i>>0) & 0x0F ] << 4;
        table[i] |=  preTable[ (i>>4) & 0x0F ] << 0;
    }
 // создаем меню и иконку
 // ОТМЕТИТЬ, ЧТО КОПИРУЕМЫЙ АДРЕС РЕСУРСА (":/../IMG/folder_explorer.ico")
 // НЕ РАБОТАЕТ, ЕГО НАДО МЕНЯТЬ

    QIcon   icon(":/IMG/folder_explorer.ico");

    // !!! обсуждаем, что такое добавление иконки
    // !!! можно сделать и в дизайнере.
    // !!! Иконка добавляется на окно приложения и на панель задач.
    // !!! Но, для установки иконки приложения на исполняемом файле
    // !!! требуется подключить файл ресурсов windows (RC-файл) "fileWatcher.rc":
    // !!! ID_ICON1               ICON    DISCARDABLE     "../IMG/folder_explorer.ico"
    setWindowIcon( icon );

    trayMenu = new QMenu(this);
    trayMenu->addAction(ui->restoreACT);
    trayMenu->addAction(ui->hideACT);
    trayMenu->addSeparator();
    trayMenu->addAction(ui->quitACT);

    // !!! здесь следует обсудить, почему объекты
    // !!! trayMenu и trayIcon можно не удалять
    trayIcon = new QSystemTrayIcon( icon, this );
    trayIcon->setContextMenu( trayMenu );
    trayIcon->show();

    QObject::connect( trayIcon,
                      SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                      this,
                      SLOT(trayClicks(QSystemTrayIcon::ActivationReason)) );
}
//------------------------------------------------------------------------------
mainWDG::~mainWDG()
{
    settings.setValue( "geometry", saveGeometry() );
    settings.setValue( "dirName", dirName );

 // удаляем выделенную память
    delete[] buf;

    delete ui;
}
//------------------------------------------------------------------------------
void mainWDG::log(const QString &s, const QString &title ) {
    // начинаем строку с текущего времени

    // начинаем строку с текущего времени
    QString out = QTime::currentTime().toString();

    out += QString( 4, ' ' );           // добавляем пробелы
    out += s;                           // добавляем сообщение
    ui->logPTE->appendPlainText( out ); // добавляем сообщение в лог
    qDebug() << out;                    // выводим в поток отладки

    if( !title.isEmpty() ) {            // выводим сообщение в трей
        trayIcon->showMessage( title, s );
    }
}
//------------------------------------------------------------------------------
void mainWDG::on_dirNamePB_clicked()
{
    QString s = QFileDialog::getExistingDirectory( this,
                                                   "Выберите директорию для слежения",
                                                   dirName );
    if( s.isEmpty() ) {
        log( "Пользователь ничего не выбрал. Оставляем все по прежнему." );
    } else {
        dirName = s;
        startWatching();
    }
}
//------------------------------------------------------------------------------
void mainWDG::startWatching()
{
 // останавливаем предыдущее слежение
    stopWatching();

    QString s = QDir::toNativeSeparators( dirName ); // !!! переменная нужна
    ui->dirNameLED->setText( s );

    log( "Устанавливаем новую директорию для слежения" );
 // запоминаем текущее содержимое директории
    fileList = QDir(dirName).entryList( QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    foreach( QString fileName, fileList ){
        log( "    " + fileName );
    }
 // запускаем слежение
    if( fsw.addPath( dirName ) ) {
        log( "+++Директория '" + s + "' добавлена+++" );
    } else {
        log( "!!!! ошибка начала слежения за '"  + s + "' !!!!" );
    }
}
//------------------------------------------------------------------------------
void mainWDG::stopWatching()
{
    // !!! тут можно объяснить разницу между  isEmpty()  и  count() > 0
    if( !fsw.directories().isEmpty() ) {
        log( "Останавливаем текущее слежение" );
        fsw.removePaths( fsw.directories() );

        fileList.clear();
    }
}
//------------------------------------------------------------------------------
void mainWDG::on_dirChange( const QString &path )
{
    log( "Изменилось содержимое директории" );
    QStringList sl = QDir(dirName).entryList( QDir::Files |
                                              QDir::Hidden |
                                              QDir::NoSymLinks );

    foreach( QString fileName, sl ){
        if( !fileList.contains(fileName)  )
        {
            log( "+++++++" + fileName, "Найден новый файл." );
            process( path  + '/' + fileName );
        }
    }
    fileList = sl;
}
//------------------------------------------------------------------------------
void mainWDG::trayClicks(QSystemTrayIcon::ActivationReason r)
{
    switch( r ) {
    case QSystemTrayIcon::DoubleClick:
    case QSystemTrayIcon::Trigger:
        if( isVisible() ) {
            hide();
        } else {
            showNormal();
            activateWindow();
        }
        break;
    default: break;
    }
}
//------------------------------------------------------------------------------
void mainWDG::hideEvent(QHideEvent *)
{
    // !!! так как наш виджет - окно верхнего уровня,
    // !!! то для него hideEvent происходит при минимизации окна
    hide();
}
//------------------------------------------------------------------------------
void mainWDG::process( const QString &name )
{
    QFile f( name );

    if(  f.open(QIODevice::ReadWrite) ) {
        int size, index, tmpI;
        qint64 pos;

        do {
            pos   = f.pos();
            size  = f.read( (char*)buf, sizeof(int)*BSIZE );
            size /= sizeof(int);

            if( size > 0 ) {
                for( int i = 0 ; i < size ; i++ ) {
                    index = (buf[i] >> 24) & 0xFF;
                    tmpI  = table[index] <<  0;

                    index = (buf[i] >> 16) & 0xFF;
                    tmpI |= table[index] <<  8;

                    index = (buf[i] >>  8) & 0xFF;
                    tmpI |= table[index] << 16;

                    index = (buf[i] >>  0) & 0xFF;
                    tmpI |= table[index] << 24;

                    buf[i] = tmpI;
                }
                f.seek( pos );
                f.write( (char*)buf, sizeof(int)*size );
            }
        } while( size == BSIZE );
    }
}
//------------------------------------------------------------------------------



