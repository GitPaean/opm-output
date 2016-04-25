/*
  Copyright 2016 Statoil ASA.

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

#include "config.h"

#if HAVE_DYNAMIC_BOOST_TEST
#define BOOST_TEST_DYN_LINK
#endif

#define BOOST_TEST_MODULE Wells
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <stdexcept>

#include <ert/ecl/ecl_sum.h>

#include <opm/output/Wells.hpp>
#include <opm/output/eclipse/Summary.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>

using namespace Opm;
using rt = data::Rates::opt;

const char* path = "summary_deck.DATA";

/* conversion factor for whenever 'day' is the unit of measure, whereas we
 * expect input in SI units (seconds)
 */
static const int day = 24 * 60 * 60;

static data::Wells result_wells() {
    /* populate with the following pattern:
     *
     * Wells are named W_1, W_2 etc, i.e. wells are 1 indexed.
     *
     * rates on a well are populated with 10 * wellidx . type (where type is
     * 0-1-2 from owg)
     *
     * bhp is wellidx.1
     * bhp is wellidx.2
     *
     * completions are 100*wellidx . type
     */

    // conversion factor Pascal (simulator output) <-> barsa
    const double ps = 100000;

    data::Rates rates1;
    rates1.set( rt::wat, 10.0 / day );
    rates1.set( rt::oil, 10.1 / day );
    rates1.set( rt::gas, 10.2 / day );

    data::Rates rates2;
    rates2.set( rt::wat, 20.0 / day );
    rates2.set( rt::oil, 20.1 / day );
    rates2.set( rt::gas, 20.2 / day );

    data::Rates rates3;
    rates3.set( rt::wat, 30.0 / day );
    rates3.set( rt::oil, 30.1 / day );
    rates3.set( rt::gas, 30.2 / day );

    data::Well well1 { rates1, 0.1 * ps, 0.2 * ps, {} };
    data::Well well2 { rates2, 1.1 * ps, 1.2 * ps, {} };
    data::Well well3 { rates3, 2.1 * ps, 2.2 * ps, {} };

    return { { "W_1", well1 }, { "W_2", well2 }, { "W_3", well3 } };
}

ERT::ert_unique_ptr< ecl_sum_type, ecl_sum_free > readsum( const std::string& base ) {
    return ERT::ert_unique_ptr< ecl_sum_type, ecl_sum_free >(
            ecl_sum_fread_alloc_case( base.c_str(), ":" )
           );
}

void rmfiles( const std::string& basename ) {
    std::remove( ( basename + ".UNSMRY" ).c_str() );
    std::remove( ( basename + ".SMSPEC" ).c_str() );
}

struct setup {
    std::shared_ptr< Deck > deck;
    EclipseState es;
    SummaryConfig config;
    data::Wells wells;
    std::string name;

    setup( const std::string& fname ) :
        deck( Parser().parseFile( path, ParseContext() ) ),
        es( deck, ParseContext() ),
        config( *deck, es ),
        wells( result_wells() ),
        name( fname )
    {}
};

/*
 * Tests works by reading the Deck, write the summary output, then immediately
 * read it again (with ERT), and compare the read values with the input.
 */
BOOST_AUTO_TEST_CASE(W_WOG_PR) {
    setup cfg( "sum_test_W_WOG_PR" );

    out::Summary writer( cfg.es, cfg.config, cfg.name );
    writer.add_timestep( 1, 1, cfg.es, cfg.wells );
    writer.write();

    auto res = readsum( cfg.name );
    const auto* resp = res.get();
    BOOST_CHECK_CLOSE( 10.0, ecl_sum_get_well_var( resp, 0, "W_1", "WWPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.0, ecl_sum_get_well_var( resp, 0, "W_2", "WWPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.0, ecl_sum_get_well_var( resp, 0, "W_3", "WWPR" ), 1e-5 );

    BOOST_CHECK_CLOSE( 10.1, ecl_sum_get_well_var( resp, 0, "W_1", "WOPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.1, ecl_sum_get_well_var( resp, 0, "W_2", "WOPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.1, ecl_sum_get_well_var( resp, 0, "W_3", "WOPR" ), 1e-5 );

    BOOST_CHECK_CLOSE( 10.2, ecl_sum_get_well_var( resp, 0, "W_1", "WGPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.2, ecl_sum_get_well_var( resp, 0, "W_2", "WGPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.2, ecl_sum_get_well_var( resp, 0, "W_3", "WGPR" ), 1e-5 );
    rmfiles( cfg.name );
}

BOOST_AUTO_TEST_CASE(W_WOG_PT) {
    setup cfg( "sum_test_W_WOG_PT" );

    out::Summary writer( cfg.es, cfg.config, cfg.name );
    writer.add_timestep( 1, 1, cfg.es, cfg.wells );
    writer.add_timestep( 2, 1, cfg.es, cfg.wells );
    writer.write();

    auto res = readsum( cfg.name );
    const auto* resp = res.get();
    BOOST_CHECK_CLOSE( 10.0 / day, ecl_sum_get_well_var( resp, 0, "W_1", "WWPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.0 / day, ecl_sum_get_well_var( resp, 0, "W_2", "WWPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.0 / day, ecl_sum_get_well_var( resp, 0, "W_3", "WWPT" ), 1e-5 );

    BOOST_CHECK_CLOSE( 10.1 / day, ecl_sum_get_well_var( resp, 0, "W_1", "WOPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.1 / day, ecl_sum_get_well_var( resp, 0, "W_2", "WOPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.1 / day, ecl_sum_get_well_var( resp, 0, "W_3", "WOPT" ), 1e-5 );

    BOOST_CHECK_CLOSE( 10.2 / day, ecl_sum_get_well_var( resp, 0, "W_1", "WGPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.2 / day, ecl_sum_get_well_var( resp, 0, "W_2", "WGPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 30.2 / day, ecl_sum_get_well_var( resp, 0, "W_3", "WGPT" ), 1e-5 );

    BOOST_CHECK_CLOSE( 2 * 10.0 / day, ecl_sum_get_well_var( resp, 1, "W_1", "WWPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 20.0 / day, ecl_sum_get_well_var( resp, 1, "W_2", "WWPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 30.0 / day, ecl_sum_get_well_var( resp, 1, "W_3", "WWPT" ), 1e-5 );

    BOOST_CHECK_CLOSE( 2 * 10.1 / day, ecl_sum_get_well_var( resp, 1, "W_1", "WOPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 20.1 / day, ecl_sum_get_well_var( resp, 1, "W_2", "WOPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 30.1 / day, ecl_sum_get_well_var( resp, 1, "W_3", "WOPT" ), 1e-5 );

    BOOST_CHECK_CLOSE( 2 * 10.2 / day, ecl_sum_get_well_var( resp, 1, "W_1", "WGPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 20.2 / day, ecl_sum_get_well_var( resp, 1, "W_2", "WGPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 30.2 / day, ecl_sum_get_well_var( resp, 1, "W_3", "WGPT" ), 1e-5 );
    rmfiles( cfg.name );
}

BOOST_AUTO_TEST_CASE(WWCT) {
    setup cfg( "sum_test_WWCT" );

    out::Summary writer( cfg.es, cfg.config, cfg.name );
    writer.add_timestep( 1, 1, cfg.es, cfg.wells );
    writer.write();

    auto res = readsum( cfg.name );
    const auto* resp = res.get();

    const double wcut1 = 10.0 / ( 10.0 + 10.1 );
    const double wcut2 = 20.0 / ( 20.0 + 20.1 );
    const double wcut3 = 30.0 / ( 30.0 + 30.1 );

    BOOST_CHECK_CLOSE( wcut1, ecl_sum_get_well_var( resp, 0, "W_1", "WWCT" ), 1e-5 );
    BOOST_CHECK_CLOSE( wcut2, ecl_sum_get_well_var( resp, 0, "W_2", "WWCT" ), 1e-5 );
    BOOST_CHECK_CLOSE( wcut3, ecl_sum_get_well_var( resp, 0, "W_3", "WWCT" ), 1e-5 );
    rmfiles( cfg.name );
}

BOOST_AUTO_TEST_CASE(WGOR) {
    setup cfg( "sum_test_WGOR" );

    out::Summary writer( cfg.es, cfg.config, cfg.name );
    writer.add_timestep( 1, 1, cfg.es, cfg.wells );
    writer.write();

    auto res = readsum( cfg.name );
    const auto* resp = res.get();

    const double wgor1 = 10.2 / 10.1;
    const double wgor2 = 20.2 / 20.1;
    const double wgor3 = 30.2 / 30.1;

    BOOST_CHECK_CLOSE( wgor1, ecl_sum_get_well_var( resp, 0, "W_1", "WGOR" ), 1e-5 );
    BOOST_CHECK_CLOSE( wgor2, ecl_sum_get_well_var( resp, 0, "W_2", "WGOR" ), 1e-5 );
    BOOST_CHECK_CLOSE( wgor3, ecl_sum_get_well_var( resp, 0, "W_3", "WGOR" ), 1e-5 );
    rmfiles( cfg.name );
}

BOOST_AUTO_TEST_CASE(WBHP) {
    setup cfg( "sum_test_WBHP" );

    out::Summary writer( cfg.es, cfg.config, cfg.name );
    writer.add_timestep( 1, 1, cfg.es, cfg.wells );
    writer.write();

    auto res = readsum( cfg.name );
    const auto* resp = res.get();

    BOOST_CHECK_CLOSE( 0.1, ecl_sum_get_well_var( resp, 0, "W_1", "WBHP" ), 1e-5 );
    BOOST_CHECK_CLOSE( 1.1, ecl_sum_get_well_var( resp, 0, "W_2", "WBHP" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2.1, ecl_sum_get_well_var( resp, 0, "W_3", "WBHP" ), 1e-5 );
    rmfiles( cfg.name );
}

BOOST_AUTO_TEST_CASE(WTHP) {
    setup cfg( "sum_test_TBHP" );

    out::Summary writer( cfg.es, cfg.config, cfg.name );
    writer.add_timestep( 1, 1, cfg.es, cfg.wells );
    writer.write();

    auto res = readsum( cfg.name );
    const auto* resp = res.get();

    BOOST_CHECK_CLOSE( 0.2, ecl_sum_get_well_var( resp, 0, "W_1", "WTHP" ), 1e-5 );
    BOOST_CHECK_CLOSE( 1.2, ecl_sum_get_well_var( resp, 0, "W_2", "WTHP" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2.2, ecl_sum_get_well_var( resp, 0, "W_3", "WTHP" ), 1e-5 );
    rmfiles( cfg.name );
}

BOOST_AUTO_TEST_CASE(WLP_R_T) {
    setup cfg( "sum_test_WLP_R_T" );

    out::Summary writer( cfg.es, cfg.config, cfg.name );
    writer.add_timestep( 1, 1, cfg.es, cfg.wells );
    writer.add_timestep( 1, 1, cfg.es, cfg.wells );
    writer.write();

    auto res = readsum( cfg.name );
    const auto* resp = res.get();

    BOOST_CHECK_CLOSE( 10.0 + 10.1, ecl_sum_get_well_var( resp, 0, "W_1", "WLPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( (10.0 + 10.1) / day, ecl_sum_get_well_var( resp, 0, "W_1", "WLPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 10.0 + 10.1, ecl_sum_get_well_var( resp, 1, "W_1", "WLPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * (10.0 + 10.1) / day, ecl_sum_get_well_var( resp, 1, "W_1", "WLPT" ), 1e-5 );

    BOOST_CHECK_CLOSE( 20.0 + 20.1, ecl_sum_get_well_var( resp, 0, "W_2", "WLPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( (20.0 + 20.1) / day, ecl_sum_get_well_var( resp, 0, "W_2", "WLPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.0 + 20.1, ecl_sum_get_well_var( resp, 1, "W_2", "WLPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * (20.0 + 20.1) / day, ecl_sum_get_well_var( resp, 1, "W_2", "WLPT" ), 1e-5 );

    BOOST_CHECK_CLOSE( 30.0 + 30.1, ecl_sum_get_well_var( resp, 0, "W_3", "WLPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( (30.0 + 30.1) / day, ecl_sum_get_well_var( resp, 0, "W_3", "WLPT" ), 1e-5 );
    BOOST_CHECK_CLOSE( (30.0 + 30.1), ecl_sum_get_well_var( resp, 1, "W_3", "WLPR" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * (30.0 + 30.1) / day, ecl_sum_get_well_var( resp, 1, "W_3", "WLPT" ), 1e-5 );

    rmfiles( cfg.name );
}

BOOST_AUTO_TEST_CASE(W_WOG_PRH) {
    setup cfg( "sum_test_W_WOG_PRH" );

    out::Summary writer( cfg.es, cfg.config, cfg.name );
    writer.add_timestep( 1, 1, cfg.es, cfg.wells );
    writer.write();

    auto res = readsum( cfg.name );
    const auto* resp = res.get();

    BOOST_CHECK_CLOSE( 10, ecl_sum_get_well_var( resp, 0, "W_1", "WWPRH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20, ecl_sum_get_well_var( resp, 0, "W_2", "WWPRH" ), 1e-5 );

    BOOST_CHECK_CLOSE( 10.1, ecl_sum_get_well_var( resp, 0, "W_1", "WOPRH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.1, ecl_sum_get_well_var( resp, 0, "W_2", "WOPRH" ), 1e-5 );

    BOOST_CHECK_CLOSE( 10.2, ecl_sum_get_well_var( resp, 0, "W_1", "WGPRH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 20.2, ecl_sum_get_well_var( resp, 0, "W_2", "WGPRH" ), 1e-5 );

    rmfiles( cfg.name );
}

BOOST_AUTO_TEST_CASE(W_WOG_PTH) {
    setup cfg( "sum_test_W_WOG_PRH" );

    out::Summary writer( cfg.es, cfg.config, cfg.name );
    writer.add_timestep( 1, 1, cfg.es, cfg.wells );
    writer.add_timestep( 1, 1, cfg.es, cfg.wells );
    writer.write();

    auto res = readsum( cfg.name );
    const auto* resp = res.get();

    BOOST_CHECK_CLOSE( 2 * 10.0 / day, ecl_sum_get_well_var( resp, 1, "W_1", "WWPTH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 20.0 / day, ecl_sum_get_well_var( resp, 1, "W_2", "WWPTH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 10.1 / day, ecl_sum_get_well_var( resp, 1, "W_1", "WOPTH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 20.1 / day, ecl_sum_get_well_var( resp, 1, "W_2", "WOPTH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 10.2 / day, ecl_sum_get_well_var( resp, 1, "W_1", "WGPTH" ), 1e-5 );
    BOOST_CHECK_CLOSE( 2 * 20.2 / day, ecl_sum_get_well_var( resp, 1, "W_2", "WGPTH" ), 1e-5 );

    rmfiles( cfg.name );
}

BOOST_AUTO_TEST_CASE(Time) {
    setup cfg( "sum_test_Time" );

    out::Summary writer( cfg.es, cfg.config, cfg.name );
    writer.add_timestep( 1, 5 *  day, cfg.es, cfg.wells );
    writer.add_timestep( 1, 5 *  day, cfg.es, cfg.wells );
    writer.add_timestep( 2, 10 * day, cfg.es, cfg.wells );
    writer.write();

    auto res = readsum( cfg.name );
    const auto* resp = res.get();

    BOOST_CHECK( ecl_sum_has_report_step( resp, 1 ) );
    BOOST_CHECK( ecl_sum_has_report_step( resp, 2 ) );
    BOOST_CHECK( !ecl_sum_has_report_step( resp, 3 ) );

    BOOST_CHECK_EQUAL( ecl_sum_iget_sim_days( resp, 0 ), 5 );
    BOOST_CHECK_EQUAL( ecl_sum_iget_sim_days( resp, 1 ), 10 );
    BOOST_CHECK_EQUAL( ecl_sum_iget_sim_days( resp, 2 ), 20 );
    BOOST_CHECK_EQUAL( ecl_sum_get_sim_length( resp ), 20 );
}

BOOST_AUTO_TEST_CASE(WRITE) {
    const auto deck = Parser().parseFile( path, ParseContext() );
    EclipseState es( deck, ParseContext() );
    SummaryConfig config( *deck, es );
    const auto wells = result_wells();

    out::Summary writer( es, config );
    writer.add_timestep( 1, 1, es, wells );
    writer.add_timestep( 1, 1, es, wells );
    writer.write();
}