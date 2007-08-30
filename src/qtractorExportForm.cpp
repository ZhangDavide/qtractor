// qtractorExportForm.cpp
//
/****************************************************************************
   Copyright (C) 2005-2007, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

#include "qtractorExportForm.h"

#include "qtractorAbout.h"
#include "qtractorAudioEngine.h"
#include "qtractorMidiEngine.h"

#include "qtractorAudioFile.h"

#include "qtractorMainForm.h"
#include "qtractorSession.h"
#include "qtractorOptions.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>


//----------------------------------------------------------------------------
// qtractorExportForm -- UI wrapper form.

// Constructor.
qtractorExportForm::qtractorExportForm (
	QWidget *pParent, Qt::WindowFlags wflags )
	: QDialog(pParent, wflags)
{
	// Setup UI struct...
	m_ui.setupUi(this);

	// Initialize dirty control state.
	m_exportType = qtractorTrack::None;
	m_pTimeScale = NULL;

	// Try to restore old window positioning.
	adjustSize();

	// UI signal/slot connections...
	QObject::connect(m_ui.ExportPathComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(stabilizeForm()));
	QObject::connect(m_ui.ExportPathToolButton,
		SIGNAL(clicked()),
		SLOT(browseExportPath()));
	QObject::connect(m_ui.SessionRangeRadioButton,
		SIGNAL(toggled(bool)),
		SLOT(rangeChanged()));
	QObject::connect(m_ui.LoopRangeRadioButton,
		SIGNAL(toggled(bool)),
		SLOT(rangeChanged()));
	QObject::connect(m_ui.EditRangeRadioButton,
		SIGNAL(toggled(bool)),
		SLOT(rangeChanged()));
	QObject::connect(m_ui.CustomRangeRadioButton,
		SIGNAL(toggled(bool)),
		SLOT(rangeChanged()));
	QObject::connect(m_ui.ExportBusNameComboBox,
		SIGNAL(activated(const QString &)),
		SLOT(stabilizeForm()));
	QObject::connect(m_ui.TimeRadioButton,
		SIGNAL(toggled(bool)),
		SLOT(formatChanged()));
	QObject::connect(m_ui.BbtRadioButton,
		SIGNAL(toggled(bool)),
		SLOT(formatChanged()));
	QObject::connect(m_ui.ExportStartSpinBox,
		SIGNAL(valueChanged(unsigned long)),
		SLOT(valueChanged()));
	QObject::connect(m_ui.ExportEndSpinBox,
		SIGNAL(valueChanged(unsigned long)),
		SLOT(valueChanged()));
	QObject::connect(m_ui.OkPushButton,
		SIGNAL(clicked()),
		SLOT(accept()));
	QObject::connect(m_ui.CancelPushButton,
		SIGNAL(clicked()),
		SLOT(reject()));
}


// Destructor.
qtractorExportForm::~qtractorExportForm (void)
{
	// Don't forget to get rid of local time-scale instance...
	if (m_pTimeScale)
		delete m_pTimeScale;
}


// Populate (setup) dialog controls from settings descriptors.
void qtractorExportForm::setExportType ( qtractorTrack::TrackType exportType )
{
	// Export type...
	m_exportType = exportType;

	QIcon icon;
	QString sExportType;
	qtractorEngine   *pEngine   = NULL;
	qtractorSession  *pSession  = NULL;
	qtractorMainForm *pMainForm = qtractorMainForm::getInstance();
	if (pMainForm)
		pSession = pMainForm->session();
	if (pSession) {
		// Copy from global time-scale instance...
		if (m_pTimeScale)
			delete m_pTimeScale;
		m_pTimeScale = new qtractorTimeScale(*pSession->timeScale());
		m_ui.ExportStartSpinBox->setTimeScale(m_pTimeScale);
		m_ui.ExportEndSpinBox->setTimeScale(m_pTimeScale);
		switch (m_exportType) {
		case qtractorTrack::Audio:
			pEngine = pSession->audioEngine();
			icon = QIcon(":/icons/trackAudio.png");
			sExportType = tr("Audio");
			break;
		case qtractorTrack::Midi:
			pEngine = pSession->midiEngine();
			icon = QIcon(":/icons/trackMidi.png");
			sExportType = tr("MIDI");
			break;
		case qtractorTrack::None:
		default:
			break;
		}
	}

	// Grab export file history, one that might me useful...
	m_ui.ExportPathComboBox->setObjectName(
		sExportType + m_ui.ExportPathComboBox->objectName());
	if (pMainForm) {
		qtractorOptions *pOptions = pMainForm->options();
		if (pOptions)
			pOptions->loadComboBoxHistory(m_ui.ExportPathComboBox);
	}

	// Fill in the output bus names list...
	m_ui.ExportBusNameComboBox->clear();
	if (pEngine) {
		QDialog::setWindowIcon(icon);
		QDialog::setWindowTitle(
			tr("Export %1").arg(sExportType) + " - " QTRACTOR_TITLE);
		for (qtractorBus *pBus = pEngine->buses().first();
				pBus; pBus = pBus->next()) {
			if (pBus->busMode() & qtractorBus::Output)
				m_ui.ExportBusNameComboBox->addItem(icon, pBus->busName());
		}
	}
	
	// Set proper time scales display format...
	if (m_pTimeScale) {
		switch (m_pTimeScale->displayFormat()) {
		case qtractorTimeScale::BBT:
			m_ui.BbtRadioButton->setChecked(true);
			break;
		case qtractorTimeScale::Time:
			m_ui.TimeRadioButton->setChecked(true);
			break;
		case qtractorTimeScale::Frames:
		default:
			m_ui.FramesRadioButton->setChecked(true);
			break;
		}
	}

	// Populate range values...
	m_ui.SessionRangeRadioButton->setChecked(true);
//	rangeChanged();

	// Done.
	stabilizeForm();
}


// Retrieve the current export type, if the case arises.
qtractorTrack::TrackType qtractorExportForm::exportType (void) const
{
	return m_exportType;
}


// Accept settings (OK button slot).
void qtractorExportForm::accept (void)
{
	qtractorMainForm *pMainForm = qtractorMainForm::getInstance();
	if (pMainForm == NULL)
		return;

	qtractorSession *pSession = pMainForm->session();
	if (pSession == NULL)
		return;

	const QString& sExportPath = m_ui.ExportPathComboBox->currentText();

	switch (m_exportType) {
	case qtractorTrack::Audio:
	{	// Audio file export...
		qtractorAudioEngine *pAudioEngine = pSession->audioEngine();
		if (pAudioEngine) {
			// Get the export bus by name...
			qtractorAudioBus *pExportBus
				= static_cast<qtractorAudioBus *> (pAudioEngine->findBus(
					m_ui.ExportBusNameComboBox->currentText()));
			// Log this event...
			pMainForm->appendMessages(
				tr("Audio file export: \"%1\" started...")
				.arg(sExportPath));
			// Do the export as commanded...
			if (pAudioEngine->fileExport(
				sExportPath,
				m_ui.ExportStartSpinBox->value(),
				m_ui.ExportEndSpinBox->value(),
				pExportBus)) {
				// Log the success...
				pMainForm->appendMessages(
					tr("Audio file export: \"%1\" complete.")
					.arg(sExportPath));
			} else {
				// Log the failure...
				pMainForm->appendMessagesError(
					tr("Audio file export:\n\n\"%1\"\n\nfailed.")
					.arg(sExportPath));
			}
		}
		break;
	}
	case qtractorTrack::Midi:
	default:
		break;
	}

	// Save other conveniency options...
	qtractorOptions *pOptions = pMainForm->options();
	if (pOptions)
		pOptions->saveComboBoxHistory(m_ui.ExportPathComboBox);

	// Just go with dialog acceptance.
	QDialog::accept();
}


// Reject settings (Cancel button slot).
void qtractorExportForm::reject (void)
{
	QDialog::reject();
}


// Choose the target filename of export.
void qtractorExportForm::browseExportPath (void)
{
	qtractorMainForm *pMainForm = qtractorMainForm::getInstance();
	if (pMainForm == NULL)
		return;

	qtractorSession *pSession = pMainForm->session();
	if (pSession == NULL)
		return;

	QString sExportPath = m_ui.ExportPathComboBox->currentText();
	if (sExportPath.isEmpty())
		sExportPath = pSession->sessionDir();

	QString sExportType;
	QString sExportExt;
	switch (m_exportType) {
	case qtractorTrack::Audio:
		sExportType = tr("Audio");
		sExportExt  = qtractorAudioFileFactory::defaultExt();
		break;
	case qtractorTrack::Midi:
		sExportType = tr("MIDI");
		sExportExt  = "mid";
		break;
	case qtractorTrack::None:
	default:
		return;
	}

	// Actual browse for the file...
	sExportPath = QFileDialog::getSaveFileName(this,
		tr("Export %1 File").arg(sExportType) + " - " QTRACTOR_TITLE,
		sExportPath, tr("%1 files (*.%2)").arg(sExportType).arg(sExportExt));

	// Have we cancelled it?
	if (sExportPath.isEmpty())
		return;

	// Enforce default file extension...
	if (QFileInfo(sExportPath).suffix() != sExportExt) {
		sExportPath += '.' + sExportExt;
		// Check wether the file already exists...
		if (QFileInfo(sExportPath).exists()) {
			if (QMessageBox::warning(this,
				tr("Warning") + " - " QTRACTOR_TITLE,
				tr("The file already exists:\n\n"
				"\"%1\"\n\n"
				"Do you want to replace it?")
				.arg(sExportPath),
				tr("Replace"), tr("Cancel")) > 0)
				return;
		}
	}

	// Finallly set as wanted...
	m_ui.ExportPathComboBox->setEditText(sExportPath);
	m_ui.ExportPathComboBox->setFocus();

	stabilizeForm();
}


// Display format has changed.
void qtractorExportForm::rangeChanged (void)
{
	qtractorMainForm *pMainForm = qtractorMainForm::getInstance();
	if (pMainForm == NULL)
		return;

	qtractorSession *pSession = pMainForm->session();
	if (pSession == NULL)
		return;

	if (m_ui.SessionRangeRadioButton->isChecked()) {
		m_ui.ExportStartSpinBox->setValue(0, false);
		m_ui.ExportEndSpinBox->setValue(pSession->sessionLength(), false);
	}
	else
	if (m_ui.LoopRangeRadioButton->isChecked()) {
		m_ui.ExportStartSpinBox->setValue(pSession->loopStart(), false);
		m_ui.ExportEndSpinBox->setValue(pSession->loopEnd(), false);
	}
	else
	if (m_ui.EditRangeRadioButton->isChecked()) {
		m_ui.ExportStartSpinBox->setValue(pSession->editHead(), false);
		m_ui.ExportEndSpinBox->setValue(pSession->editTail(), false);
	}

	stabilizeForm();
}


// Range values have changed.
void qtractorExportForm::valueChanged (void)
{
	m_ui.CustomRangeRadioButton->setChecked(true);

//	stabilizeForm();
}


// Display format has changed.
void qtractorExportForm::formatChanged (void)
{
	qtractorTimeScale::DisplayFormat displayFormat = qtractorTimeScale::Frames;

	if (m_ui.TimeRadioButton->isChecked())
		displayFormat = qtractorTimeScale::Time;
	else
	if (m_ui.BbtRadioButton->isChecked())
		displayFormat= qtractorTimeScale::BBT;

	if (m_pTimeScale) {
		// Set from local time-scale instance...
		m_pTimeScale->setDisplayFormat(displayFormat);
		m_ui.ExportStartSpinBox->updateDisplayFormat();
		m_ui.ExportEndSpinBox->updateDisplayFormat();
	}

	stabilizeForm();
}


// Stabilize current form state.
void qtractorExportForm::stabilizeForm (void)
{
	qtractorMainForm *pMainForm = qtractorMainForm::getInstance();
	if (pMainForm == NULL)
		return;

	qtractorSession *pSession = pMainForm->session();
	if (pSession == NULL)
		return;

	m_ui.LoopRangeRadioButton->setEnabled(
		pSession->loopStart() < pSession->loopEnd());
	m_ui.EditRangeRadioButton->setEnabled(
		pSession->editHead() < pSession->editTail());

	m_ui.OkPushButton->setEnabled(
		!m_ui.ExportPathComboBox->currentText().isEmpty() &&
		m_ui.ExportStartSpinBox->value() < m_ui.ExportEndSpinBox->value());
}


// end of qtractorExportForm.cpp
