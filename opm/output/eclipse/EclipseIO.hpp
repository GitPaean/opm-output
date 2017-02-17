/*
  Copyright (c) 2013 Andreas Lauser
  Copyright (c) 2013 Uni Research AS
  Copyright (c) 2014 IRIS AS

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef OPM_ECLIPSE_WRITER_HPP
#define OPM_ECLIPSE_WRITER_HPP

#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/NNC.hpp>

#include <string>
#include <vector>
#include <array>
#include <memory>

#include <opm/output/data/Cells.hpp>
#include <opm/output/data/Solution.hpp>
#include <opm/output/data/Wells.hpp>

namespace Opm {

class EclipseState;

/*!
 * \brief A class to write the reservoir state and the well state of a
 *        blackoil simulation to disk using the Eclipse binary format.
 */
class EclipseIO {
public:
    /*!
     * \brief Sets the common attributes required to write eclipse
     *        binary files using ERT.
     */
    EclipseIO( const EclipseState&, EclipseGrid );




/**    \brief Output static properties in EGRID and INIT file.
  *
  *    Write the static eclipse data (grid, PVT curves, etc) to disk,
  *    and set up additional initial properties. There are no specific
  *    keywords to configure the output to the INIT files, it seems
  *    like the algorithm is: "All available keywords are written to
  *    INIT file". For the current code the algorithm is as such:
  *
  *
  *    1. 3D properties which can be simply calculated in the output
  *       layer are unconditionally written to disk, that currently
  *       includes the DX, DY, DZ and DEPTH keyword.
  *
  *    2. All integer propeerties from the deck are written
  *       unconditionally, that typically includes the MULTNUM, PVTNUM,
  *       SATNUM and so on. Observe that the keywords PVTNUM, SATNUM,
  *       EQLNUM and FIPNUM are autocreated in the output layer, so
  *       they will be on disk even if they are not explicitly included
  *       in the deck.
  *
  *    3. The PORV keyword will *always* be present in the INIT file,
  *       and that keyword will have nx*ny*nz elements; all other 3D
  *       properties will only have nactive elements.
  *
  *    4. For floating point 3D keywords from the deck - like PORO and
  *       PERMX there is a *hardcoded* list in the writeINIFile( )
  *       implementation of which keywords to output - if they are
  *       available.
  *
  *    5. The container simProps contains additional 3D floating point
  *       properties which have been calculated by the simulator, this
  *       typically includes the transmissibilities TRANX, TRANY and
  *       TRANZ but could in principle be anye floating point
  *       property.
  *
  *       If you want the FOE keyword in the summary output the
  *       simProps container must contain the initial OIP field.
  *
  *   In addition:
  *
  *   - The NNC argument is distributed between the EGRID and INIT
  *     files.
  *
  *   - PVT curves are written unconditionally, saturation functions
  *     are not yet written to disk.
  */

    void writeInitial( data::Solution simProps = data::Solution(), const NNC& nnc = NNC());

    /*!
     * \brief Write a reservoir state and summary information to disk.
     *
     *
     * The reservoir state can be inspected with visualization tools like
     * ResInsight.
     *
     * The summary information can then be visualized using tools from
     * ERT or ECLIPSE. Note that calling this method is only
     * meaningful after the first time step has been completed.
     *
     * The optional simProps vector contains fields which have been
     * calculated by the simulator and are written to the restart
     * file. Examples of such fields would be the relative
     * permeabilities KRO, KRW and KRG and fluxes. The keywords which
     * can be added here are represented with mnenonics in the RPTRST
     * keyword.
     */

    void writeTimeStep( int report_step,
                        bool isSubstep,
                        double seconds_elapsed,
                        data::Solution,
                        data::Wells);


    /*
      Will load solution data and wellstate from the restart
      file. This method will consult the IOConfig object to get
      filename and report step to restart from.

      The map keys should be a map of keyword names and their
      corresponding dimension object, i.e. to load the state from a
      simple two phase simulation you would pass:

         keys = {{"PRESSURE" , UnitSystem::measure::pressure},
                 {"SWAT" , UnitSystem::measure::identity }}

      For a three phase black oil simulation you would add pairs for
      SGAS, RS and RV. If you ask for keys which are not found in the
      restart file an exception will be raised, the happens if the
      size of a vector is wrong.

      The function will consult the InitConfig object in the
      EclipseState object to determine which file and report step to
      load.

      The return value is of type 'data::Solution', which is the same
      container type which is used by the EclipseIO, but observe
      that the dim and target elements carry no information:

         - The returned double data has been converted to SI.
         . The target is unconditionally set to 'RESTART_SOLUTION'
    */
    std::pair< data::Solution, data::Wells >
    loadRestart(const std::map<std::string, UnitSystem::measure>& keys) const;


    EclipseIO( const EclipseIO& ) = delete;
    ~EclipseIO();

private:
    class Impl;
    std::unique_ptr< Impl > impl;

};

} // namespace Opm


#endif // OPM_ECLIPSE_WRITER_HPP