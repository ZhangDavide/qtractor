// qtractorDssiPlugin.cpp
//
/****************************************************************************
   Copyright (C) 2005-2008, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "qtractorAbout.h"

#ifdef CONFIG_DSSI

#include "qtractorDssiPlugin.h"


//----------------------------------------------------------------------------
// qtractorDssiPluginType -- VST plugin type instance.
//

// Derived methods.
bool qtractorDssiPluginType::open (void)
{
	// Do we have a descriptor already?
	if (m_pDssiDescriptor == NULL)
		m_pDssiDescriptor = dssi_descriptor(file(), index());
	if (m_pDssiDescriptor == NULL)
		return false;

	// We're also a LADSPA one...
	m_pLadspaDescriptor = m_pDssiDescriptor->LADSPA_Plugin;

	// Let's get the it's own LADSPA stuff...
	if (!qtractorLadspaPluginType::open()) {
		m_pLadspaDescriptor = NULL;
		m_pDssiDescriptor = NULL;
		return false;
	}

#ifdef CONFIG_DEBUG
	qDebug("qtractorDssiPluginType[%p]::open() filename=\"%s\" index=%lu",
		this, file()->filename().toUtf8().constData(), index());
#endif

	// Things we have now for granted...
	m_iMidiIns = 1;

	// Check for GUI editor exacutable...
	QFileInfo fi(file()->filename());
	QFileInfo gi(fi.dir(), fi.baseName());
	if (gi.isDir()) {
		QDir dir(gi.absoluteFilePath(), fi.baseName() + "_*");
		QStringList guis = dir.entryList(QDir::Files | QDir::Executable);
		if (!guis.isEmpty()) {
			gi.setFile(dir, guis.first()); // FIXME: Only the first?
			m_sDssiEditor = gi.absoluteFilePath();
			m_bEditor = gi.isExecutable();
		}
    }

	return true;
}


void qtractorDssiPluginType::close (void)
{
	m_pDssiDescriptor = NULL;
	qtractorLadspaPluginType::close();
}


// Factory method (static)
qtractorDssiPluginType *qtractorDssiPluginType::createType (
	qtractorPluginFile *pFile, unsigned long iIndex )
{
	// Sanity check...
	if (pFile == NULL)
		return NULL;

	// Retrieve DSSI descriptor if any...
	const DSSI_Descriptor *pDssiDescriptor = dssi_descriptor(pFile, iIndex);
	if (pDssiDescriptor == NULL)
		return NULL;

	// Yep, most probably its a valid plugin descriptor...
	return new qtractorDssiPluginType(pFile, iIndex, pDssiDescriptor);
}


// Descriptor method (static)
const DSSI_Descriptor *qtractorDssiPluginType::dssi_descriptor (
	qtractorPluginFile *pFile, unsigned long iIndex )
{
	// Retrieve the DSSI descriptor function, if any...
	DSSI_Descriptor_Function pfnDssiDescriptor
		= (DSSI_Descriptor_Function) pFile->resolve("dssi_descriptor");
	if (pfnDssiDescriptor == NULL)
		return NULL;

	// Retrieve DSSI descriptor if any...
	return (*pfnDssiDescriptor)(iIndex);
}


//----------------------------------------------------------------------------
// qtractorDssiPlugin -- VST plugin instance.
//

// Constructors.
qtractorDssiPlugin::qtractorDssiPlugin ( qtractorPluginList *pList,
	qtractorDssiPluginType *pDssiType )
	: qtractorLadspaPlugin(pList, pDssiType)
{
}


// Destructor.
qtractorDssiPlugin::~qtractorDssiPlugin (void)
{
}


// Channel/instance number accessors.
void qtractorDssiPlugin::setChannels ( unsigned short iChannels )
{
	qtractorLadspaPlugin::setChannels(iChannels);
}


// Do the actual activation.
void qtractorDssiPlugin::activate (void)
{
	qtractorLadspaPlugin::activate();
}


// Do the actual deactivation.
void qtractorDssiPlugin::deactivate (void)
{
	qtractorLadspaPlugin::deactivate();
}


// The main plugin processing procedure.
void qtractorDssiPlugin::process (
	float **ppIBuffer, float **ppOBuffer, unsigned int nframes )
{
	qtractorLadspaPlugin::process(ppIBuffer, ppOBuffer, nframes);
}


// GUI Editor stuff.
void qtractorDssiPlugin::openEditor ( QWidget */*pParent*/ )
{
	qtractorDssiPluginType *pDssiType
		= static_cast<qtractorDssiPluginType *> (type());
	if (pDssiType == NULL)
		return;
	if (!pDssiType->isEditor())
		return;

#if 0

	QFileInfo fi((pDssiType->file())->filename());
	QByteArray aDssiEditor = pDssiType->dssi_editor().toUtf8();
	QByteArray aOscPath  = g_sOscPath.toUtf8();
	QByteArray aFilename = fi.fileName().toUtf8();
	QByteArray aLabel    = pDssiType->label().toUtf8();
	QByteArray aName     = pDssiType->name().toUtf8();

	const char *pszDssiEditor = aDssiEditor.constData();
	const char *pszOscPath  = aOscPath.constData();
	const char *pszFilename = aFilename.constData();
	const char *pszLabel    = aLabel.constData();
	const char *pszName     = aName.constData();

#ifdef CONFIG_DEBUG
	qDebug("qtractorDssiPlugin::openEditor() "
		"path=\"%s\" url=\"%s\" filename=\"%s\" label=\"%s\" name=\"%s\"",
		pszDssiEditor, pszOscPath, pszFilename, pszLabel, pszName);
#endif

	if (::fork() == 0) {
		::execlp(pszDssiEditor,
			pszDssiEditor, pszOscPath, pszFilename, pszLabel, pszName, NULL);
		::perror("exec failed");
		::exit(1);
	}

#endif
}


//----------------------------------------------------------------------------
// qtractorDssiPluginParam -- VST plugin control input port instance.
//

// Constructors.
qtractorDssiPluginParam::qtractorDssiPluginParam (
	qtractorDssiPlugin *pDssiPlugin, unsigned long iIndex )
	: qtractorLadspaPluginParam(pDssiPlugin, iIndex)
{
}


// Destructor.
qtractorDssiPluginParam::~qtractorDssiPluginParam (void)
{
}

#endif	// CONFIG_DSSI

// end of qtractorDssiPlugin.cpp
