// main.cpp — Qt entry point of the SurMap5 editor port.
//
// The old entry was CSurMap5App::InitInstance (SurMap5/SurMap5.cpp), an MFC
// CWinApp. Here QApplication owns the loop; the editor-wide initialization
// moves to EditorApplication, the frame to MainWindow, and the repaint-driven
// game loop (MFC OnIdle -> CMainFrame::universeQuant -> view Invalidate)
// becomes a QTimer tick connected to MainWindow::universeQuant.

#include <QApplication>

#include "EditorApplication.h"
#include "MainWindow.h"

int main(int argc, char** argv)
{
	QApplication app(argc, argv);
	QApplication::setOrganizationName("VistaGame");
	QApplication::setApplicationName("VistaEngine SurMap5");

	EditorApplication editor;
	if(!editor.initialize())
		return 1;

	MainWindow window;
	window.showMaximized();

	// The editor's game loop, ~60 Hz. universeQuant repaints the 3D viewport
	// (and, once the engine is linked, runs logic at the logic time period the
	// way CSurMap5App::OnIdle used to).
	QObject::connect(&editor.loopTimer(), &QTimer::timeout,
	                 &window, &MainWindow::universeQuant);

	return app.exec();
}
