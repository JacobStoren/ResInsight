/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2019-     Equinor ASA
//
//  ResInsight is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  ResInsight is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.
//
//  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
//  for more details.
//
/////////////////////////////////////////////////////////////////////////////////
#include "RimGridPlotWindow.h"

#include "RimPlotInterface.h"
#include "RiuPlotMainWindow.h"
#include "RiuPlotMainWindowTools.h"

#include <QRegularExpression>

#include <cvfAssert.h>

namespace caf
{
template <>
void RimGridPlotWindow::ColumnCountEnum::setUp()
{
    addItem( RimGridPlotWindow::COLUMNS_1, "1", "1 Column" );
    addItem( RimGridPlotWindow::COLUMNS_2, "2", "2 Columns" );
    addItem( RimGridPlotWindow::COLUMNS_3, "3", "3 Columns" );
    addItem( RimGridPlotWindow::COLUMNS_4, "4", "4 Columns" );
    addItem( RimGridPlotWindow::COLUMNS_UNLIMITED, "UNLIMITED", "Unlimited" );
    setDefault( RimGridPlotWindow::COLUMNS_UNLIMITED );
}
} // namespace caf

CAF_PDM_SOURCE_INIT( RimGridPlotWindow, "GridPlotWindow" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimGridPlotWindow::RimGridPlotWindow()
{
    CAF_PDM_InitObject( "Grid Plot Window", ":/WellLogPlot16x16.png", "", "" );

    CAF_PDM_InitFieldNoDefault( &m_plots, "Tracks", "", "", "", "" );
    m_plots.uiCapability()->setUiHidden( true );

    CAF_PDM_InitFieldNoDefault( &m_columnCountEnum, "NumberOfColumns", "Number of Columns", "", "", "" );

    m_viewer = nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimGridPlotWindow::~RimGridPlotWindow()
{
    removeMdiWindowFromMdiArea();
    m_plots.deleteAllChildObjects();

    deleteViewWidget();
}

//--------------------------------------------------------------------------------------------------
/// Move-assignment operator. Argument has to be passed with std::move()
//--------------------------------------------------------------------------------------------------
RimGridPlotWindow& RimGridPlotWindow::operator=( RimGridPlotWindow&& rhs )
{
    RimPlotWindow::operator=( std::move( rhs ) );

    // Move all tracks
    std::vector<caf::PdmObject*> plots = rhs.m_plots.childObjects();
    rhs.m_plots.clear();
    for ( caf::PdmObject* plot : plots )
    {
        CAF_ASSERT( dynamic_cast<RimPlotInterface*>( plot ) );
        m_plots.push_back( plot );
    }

    m_columnCountEnum = rhs.m_columnCountEnum;

    return *this;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QWidget* RimGridPlotWindow::viewWidget()
{
    return m_viewer;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGridPlotWindow::addPlot( RimPlotInterface* plot )
{
    insertPlot( plot, m_plots.size() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGridPlotWindow::insertPlot( RimPlotInterface* plot, size_t index )
{
    if ( plot )
    {
        m_plots.insert( index, toPdmObjectAsserted( plot ) );

        if ( m_viewer )
        {
            plot->createPlotWidget();
            m_viewer->insertPlot( plot->viewer(), index );
        }

        onPlotAdditionOrRemoval();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGridPlotWindow::removePlot( RimPlotInterface* plot )
{
    if ( plot )
    {
        if ( m_viewer )
        {
            m_viewer->removePlot( plot->viewer() );
        }
        m_plots.removeChildObject( toPdmObjectAsserted( plot ) );

        onPlotAdditionOrRemoval();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
size_t RimGridPlotWindow::plotCount() const
{
    return m_plots.size();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
size_t RimGridPlotWindow::plotIndex( const RimPlotInterface* plot ) const
{
    return m_plots.index( toPdmObjectAsserted( plot ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimPlotInterface* RimGridPlotWindow::plotByIndex( size_t index ) const
{
    return toPlotInterfaceAsserted( m_plots[index] );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RimPlotInterface*> RimGridPlotWindow::plots() const
{
    std::vector<RimPlotInterface*> allPlots;
    allPlots.reserve( m_plots.size() );

    for ( caf::PdmObject* pdmObject : m_plots )
    {
        allPlots.push_back( toPlotInterfaceAsserted( pdmObject ) );
    }
    return allPlots;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RimPlotInterface*> RimGridPlotWindow::visiblePlots() const
{
    std::vector<RimPlotInterface*> allPlots;
    for ( caf::PdmObject* pdmObject : m_plots() )
    {
        RimPlotInterface* plot = toPlotInterfaceAsserted( pdmObject );
        if ( plot->isChecked() )
        {
            allPlots.push_back( plot );
        }
    }
    return allPlots;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGridPlotWindow::updateLayout()
{
    if ( m_showWindow )
    {
        m_viewer->scheduleUpdate();
    }
}

#if 0
//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGridPlotWindow::updatePlotNames()
{
    auto plotVector = plots();
    for ( size_t tIdx = 0; tIdx < plotVector.size(); ++tIdx )
    {
        QString            description = plotVector[tIdx]->description();
        QRegularExpression regexp( "Track \\d+" );
        description.replace( regexp, QString( "Track %1" ).arg( tIdx + 1 ) );
        plotVector[tIdx]->setDescription( description );
    }
}
#endif

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGridPlotWindow::updatePlotOrderFromGridWidget()
{
    std::sort( m_plots.begin(), m_plots.end(), [this]( caf::PdmObject* lhs, caf::PdmObject* rhs ) {
        auto indexLhs = m_viewer->indexOfPlotWidget( toPlotInterfaceAsserted( lhs )->viewer() );
        auto indexRhs = m_viewer->indexOfPlotWidget( toPlotInterfaceAsserted( rhs )->viewer() );
        return indexLhs < indexRhs;
    } );
    //    updatePlotNames();
    updateConnectedEditors();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGridPlotWindow::setAutoScaleXEnabled( bool enabled )
{
    for ( RimPlotInterface* plot : plots() )
    {
        plot->setAutoScaleXEnabled( enabled );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGridPlotWindow::setAutoScaleYEnabled( bool enabled )
{
    for ( RimPlotInterface* plot : plots() )
    {
        plot->setAutoScaleYEnabled( enabled );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RimGridPlotWindow::columnCount() const
{
    if ( m_columnCountEnum() == COLUMNS_UNLIMITED )
    {
        return static_cast<int>( visiblePlots().size() );
    }
    return static_cast<int>( m_columnCountEnum() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::PdmFieldHandle* RimGridPlotWindow::columnCountField()
{
    return &m_columnCountEnum;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGridPlotWindow::zoomAll()
{
    setAutoScaleXEnabled( true );
    setAutoScaleYEnabled( true );
    updateZoom();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::PdmUiGroup* RimGridPlotWindow::createPlotSettingsUiGroup( caf::PdmUiOrdering& uiOrdering )
{
    caf::PdmUiGroup* titleAndLegendsGroup = RimPlotWindow::createPlotSettingsUiGroup( uiOrdering );
    titleAndLegendsGroup->add( &m_columnCountEnum );
    return titleAndLegendsGroup;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimGridPlotWindow::asciiDataForPlotExport() const
{
    QString out = description() + "\n";

    for ( RimPlotInterface* plot : plots() )
    {
        if ( plot->isChecked() )
        {
            out += plot->asciiDataForPlotExport();
        }
    }

    return out;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGridPlotWindow::onPlotAdditionOrRemoval()
{
    // updatePlotNames();
    updateColumnCount();
    updateConnectedEditors();
    updateLayout();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QImage RimGridPlotWindow::snapshotWindowContent()
{
    QImage image;

    if ( m_viewer )
    {
        m_viewer->setScrollbarVisible( false );
        m_viewer->setSelectionsVisible( false );
        QPixmap pix = m_viewer->grab();
        image       = pix.toImage();
        m_viewer->setScrollbarVisible( true );
        m_viewer->setSelectionsVisible( true );
    }

    return image;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QWidget* RimGridPlotWindow::createViewWidget( QWidget* mainWindowParent )
{
    m_viewer = new RiuGridPlotWindow( this, mainWindowParent );
    recreatePlotWidgets();
    return m_viewer;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGridPlotWindow::deleteViewWidget()
{
    detachAllCurves();

    if ( m_viewer )
    {
        m_viewer->deleteLater();
        m_viewer = nullptr;
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGridPlotWindow::fieldChangedByUi( const caf::PdmFieldHandle* changedField,
                                          const QVariant&            oldValue,
                                          const QVariant&            newValue )
{
    RimPlotWindow::fieldChangedByUi( changedField, oldValue, newValue );

    if ( changedField == &m_columnCountEnum )
    {
        updateLayout();
        updateColumnCount();
    }
    updateConnectedEditors();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGridPlotWindow::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    createPlotSettingsUiGroup( uiOrdering );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimGridPlotWindow::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions,
                                                                        bool*                      useOptionsOnly )
{
    QList<caf::PdmOptionItemInfo> options = RimPlotWindow::calculateValueOptions( fieldNeedingOptions, useOptionsOnly );

    if ( fieldNeedingOptions == &m_columnCountEnum )
    {
        for ( size_t i = 0; i < ColumnCountEnum::size(); ++i )
        {
            ColumnCount enumVal = ColumnCountEnum::fromIndex( i );
            if ( enumVal == COLUMNS_UNLIMITED )
            {
                QString iconPath( ":/ColumnsUnlimited.png" );
                options.push_back( caf::PdmOptionItemInfo( ColumnCountEnum::uiText( enumVal ),
                                                           enumVal,
                                                           false,
                                                           caf::QIconProvider( iconPath ) ) );
            }
            else
            {
                QString iconPath = QString( ":/Columns%1.png" ).arg( static_cast<int>( enumVal ) );
                options.push_back( caf::PdmOptionItemInfo( ColumnCountEnum::uiText( enumVal ),
                                                           enumVal,
                                                           false,
                                                           caf::QIconProvider( iconPath ) ) );
            }
        }
    }
    return options;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGridPlotWindow::onLoadDataAndUpdate()
{
    updateMdiWindowVisibility();
    setPlotTitleInWidget( this->fullPlotTitle() );
    updatePlots();
    updateLayout();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGridPlotWindow::initAfterRead()
{
    RimPlotWindow::initAfterRead();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGridPlotWindow::updatePlotTitle()
{
    m_viewer->setTitleVisible( m_showTitleInPlot() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGridPlotWindow::setPlotTitleInWidget( const QString& title )
{
    if ( m_viewer )
    {
        m_viewer->setPlotTitle( title );
    }
    updateMdiWindowTitle();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGridPlotWindow::updatePlots()
{
    if ( m_showWindow )
    {
        for ( RimPlotInterface* plot : plots() )
        {
            plot->loadDataAndUpdate();
        }
        this->updateZoom();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGridPlotWindow::updateZoom()
{
    for ( RimPlotInterface* plot : plots() )
    {
        plot->updateZoomInQwt();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGridPlotWindow::recreatePlotWidgets()
{
    CVF_ASSERT( m_viewer );

    auto plotVector = plots();

    for ( size_t tIdx = 0; tIdx < plotVector.size(); ++tIdx )
    {
        plotVector[tIdx]->createPlotWidget();
        m_viewer->addPlot( plotVector[tIdx]->viewer() );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimGridPlotWindow::hasCustomFontSizes( RiaDefines::FontSettingType fontSettingType, int defaultFontSize ) const
{
    if ( fontSettingType == RiaDefines::PLOT_FONT && m_viewer )
    {
        if ( m_viewer->fontSize() != defaultFontSize )
        {
            return true;
        }
        for ( const RimPlotInterface* plot : plots() )
        {
            if ( plot->hasCustomFontSizes( fontSettingType, defaultFontSize ) )
            {
                return true;
            }
        }
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimGridPlotWindow::applyFontSize( RiaDefines::FontSettingType fontSettingType,
                                       int                         oldFontSize,
                                       int                         fontSize,
                                       bool                        forceChange /*= false */ )
{
    bool somethingChanged = false;
    if ( fontSettingType == RiaDefines::PLOT_FONT && m_viewer )
    {
        if ( oldFontSize == m_viewer->fontSize() || forceChange )
        {
            m_viewer->setFontSize( fontSize );
            somethingChanged = true;
        }

        for ( RimPlotInterface* plot : plots() )
        {
            if ( plot->applyFontSize( fontSettingType, oldFontSize, fontSize, forceChange ) )
            {
                somethingChanged = true;
            }
        }
        if ( somethingChanged )
        {
            m_viewer->scheduleUpdate();
        }
    }
    return somethingChanged;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGridPlotWindow::detachAllCurves()
{
    auto plotVector = plots();
    for ( size_t tIdx = 0; tIdx < plotVector.size(); ++tIdx )
    {
        plotVector[tIdx]->detachAllCurves();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGridPlotWindow::updateColumnCount()
{
    RiuPlotMainWindowTools::refreshToolbars();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimPlotInterface* RimGridPlotWindow::toPlotInterfaceAsserted( caf::PdmObject* pdmObject )
{
    RimPlotInterface* plotInterface = dynamic_cast<RimPlotInterface*>( pdmObject );
    CAF_ASSERT( plotInterface );
    return plotInterface;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const RimPlotInterface* RimGridPlotWindow::toPlotInterfaceAsserted( const caf::PdmObject* pdmObject )
{
    const RimPlotInterface* plotInterface = dynamic_cast<const RimPlotInterface*>( pdmObject );
    CAF_ASSERT( plotInterface );
    return plotInterface;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::PdmObject* RimGridPlotWindow::toPdmObjectAsserted( RimPlotInterface* plotInterface )
{
    caf::PdmObject* pdmObject = dynamic_cast<caf::PdmObject*>( plotInterface );
    CAF_ASSERT( pdmObject );
    return pdmObject;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const caf::PdmObject* RimGridPlotWindow::toPdmObjectAsserted( const RimPlotInterface* plotInterface )
{
    const caf::PdmObject* pdmObject = dynamic_cast<const caf::PdmObject*>( plotInterface );
    CAF_ASSERT( pdmObject );
    return pdmObject;
}