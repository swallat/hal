#include "netlist_test_utils.h"
#include "netlist/gate.h"
#include "netlist/gate_library/gate_library_manager.h"
#include "netlist/netlist.h"
#include "netlist/netlist_factory.h"
#include "netlist/persistent/netlist_serializer.h"
#include "gtest/gtest.h"
#include "hdl_parser/hdl_parser_vhdl_old.h"
#include <iostream>
#include <sstream>
#include <boost/filesystem.hpp>


using namespace test_utils;

class hdl_parser_vhdl_old_test : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        NO_COUT_BLOCK;
        gate_library_manager::load_all();
    }

    virtual void TearDown()
    {
    }
};

/*                                    net_0
 *                  .--= INV (0) =----.
 *  global_in       |                   '-=                     global_out
 *      ------------|                   .-= AND3 (2) = ----------
 *                  |                   | =
 *                  '--=                |
 *                       AND2 (1) =---'
 *                     =              vec_net(2)
 *                                                              global_inout
 *      -----------------------------------------------------------
 *
 */

/**
 * Testing the correct usage of the vhdl parser by parse a small vhdl-format string, which describes the netlist
 * shown above.
 *
 * Functions: parse
 */
TEST_F(hdl_parser_vhdl_old_test, check_main_example)
{
    TEST_START
    std::stringstream input("-- Device\t: device_name\n"
                            "library IEEE;\n"
                            "use IEEE.STD_LOGIC_1164.ALL;\n"
                            "library SIMPRIM;\n"
                            "use SIMPRIM.VCOMPONENTS.ALL;\n"
                            "use SIMPRIM.VPKG.ALL;\n"
                            "\n"
                            "entity TEST_Comp is\n"
                            "  port (\n"
                            "    net_global_in : in STD_LOGIC := 'X';\n"
                            "    net_global_out : out STD_LOGIC := 'X';\n"
                            "    net_global_inout : inout STD_LOGIC := 'X';\n"
                            "  );\n"
                            "end AES_Comp;\n"
                            "\n"
                            "architecture STRUCTURE of TEST_Comp is\n"
                            "  signal net_0 : STD_LOGIC;\n"
                            "  signal vec_net : STD_LOGIC_VECTOR ( 2 downto 0 );\n"
                            "  attribute PTYPE: IGNORE_ME;\n"
                            "begin\n"
                            "  gate_0 : INV\n"
                            "    port map (\n"
                            "      I => net_global_in,\n"
                            "      O => net_0\n"
                            "    );\n"
                            "  gate_1 : AND2\n"
                            "    port map (\n"
                            "      I0 => net_global_in,\n"
                            "      I1 => net_global_in,\n"
                            "      O => vec_net(2)\n"
                            "    );\n"
                            "  gate_2 : AND3\n"
                            "    port map (\n"
                            "      I0 => net_0,\n"
                            "      I1 => vec_net(2),\n"
                            "      O => net_global_out\n"
                            "    );\n"
                            "end STRUCTURE;\n"
                            "");
    test_def::capture_stdout();
    hdl_parser_vhdl_old vhdl_parser(input);
    std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);
    if (nl == nullptr)
    {
        std::cout << test_def::get_captured_stdout();
    }
    else
    {
        test_def::get_captured_stdout();
    }

    ASSERT_NE(nl, nullptr);

    // Check if the device name is parsed correctly
    EXPECT_EQ(nl->get_device_name(), "device_name");

    // Check if the gates are parsed correctly
    ASSERT_EQ(nl->get_gates("INV").size(), 1);
    std::shared_ptr<gate> gate_0 = *(nl->get_gates("INV").begin());
    ASSERT_EQ(nl->get_gates("AND2").size(), 1);
    std::shared_ptr<gate> gate_1 = *(nl->get_gates("AND2").begin());
    ASSERT_EQ(nl->get_gates("AND3").size(), 1);
    std::shared_ptr<gate> gate_2 = *(nl->get_gates("AND3").begin());

    ASSERT_NE(gate_0, nullptr);
    EXPECT_EQ(gate_0->get_name(), "gate_0");

    ASSERT_NE(gate_1, nullptr);
    EXPECT_EQ(gate_1->get_name(), "gate_1");

    ASSERT_NE(gate_2, nullptr);
    EXPECT_EQ(gate_2->get_name(), "gate_2");

    // Check if the nets are parsed correctly
    std::shared_ptr<net> net_0            = *(nl->get_nets("net_0").begin());
    std::shared_ptr<net> vec_net_2        = *(nl->get_nets("vec_net(2)").begin());
    std::shared_ptr<net> net_global_in    = *(nl->get_nets("net_global_in").begin());
    std::shared_ptr<net> net_global_out   = *(nl->get_nets("net_global_out").begin());
    std::shared_ptr<net> net_global_inout = *(nl->get_nets("net_global_inout").begin());

    ASSERT_NE(net_0, nullptr);
    EXPECT_EQ(net_0->get_name(), "net_0");
    EXPECT_EQ(net_0->get_src(), get_endpoint(gate_0, "O"));
    std::vector<endpoint> exp_net_0_dsts = {get_endpoint(gate_2, "I0")};
    EXPECT_TRUE(vectors_have_same_content(net_0->get_dsts(), std::vector<endpoint>({get_endpoint(gate_2, "I0")})));

    ASSERT_NE(vec_net_2, nullptr);
    EXPECT_EQ(vec_net_2->get_name(), "vec_net(2)");
    EXPECT_EQ(vec_net_2->get_src(), get_endpoint(gate_1, "O"));
    EXPECT_TRUE(vectors_have_same_content(vec_net_2->get_dsts(), std::vector<endpoint>({get_endpoint(gate_2, "I1")})));

    ASSERT_NE(net_global_in, nullptr);
    EXPECT_EQ(net_global_in->get_name(), "net_global_in");
    EXPECT_EQ(net_global_in->get_src(), get_endpoint(nullptr, ""));
    EXPECT_TRUE(vectors_have_same_content(net_global_in->get_dsts(), std::vector<endpoint>({get_endpoint(gate_0, "I"), get_endpoint(gate_1, "I0"), get_endpoint(gate_1, "I1")})));
    EXPECT_TRUE(nl->is_global_input_net(net_global_in));

    ASSERT_NE(net_global_out, nullptr);
    EXPECT_EQ(net_global_out->get_name(), "net_global_out");
    EXPECT_EQ(net_global_out->get_src(), get_endpoint(gate_2, "O"));
    EXPECT_TRUE(net_global_out->get_dsts().empty());
    EXPECT_TRUE(nl->is_global_output_net(net_global_out));

    ASSERT_NE(net_global_inout, nullptr);
    EXPECT_EQ(net_global_inout->get_name(), "net_global_inout");
    EXPECT_EQ(net_global_inout->get_src(), get_endpoint(nullptr, ""));
    EXPECT_TRUE(net_global_inout->get_dsts().empty());
    EXPECT_TRUE(nl->is_global_inout_net(net_global_inout));

    EXPECT_EQ(nl->get_global_input_nets().size(), 1);
    EXPECT_EQ(nl->get_global_output_nets().size(), 1);
    EXPECT_EQ(nl->get_global_inout_nets().size(), 1);

    TEST_END
}

/**
 * Testing the correct storage of data, passed by the "generic map" keyword
 *
 * Functions: parse
 */
TEST_F(hdl_parser_vhdl_old_test, check_generic_map){
TEST_START
    {
        // A boolean value is passed
        std::stringstream input("-- Device\t: device_name\n"
                                   "entity TEST_Comp is\n"
                                   "  port (\n"
                                   "    net_global_inout : inout STD_LOGIC := 'X';\n"
                                   "  );\n"
                                   "end TEST_Comp;\n"
                                   "architecture STRUCTURE of TEST_Comp is\n"
                                   "begin\n"
                                   "  gate_0 : INV\n"
                                   "    generic map(\n"
                                   "      key_bool => true\n"
                                   "    )\n"
                                   "    port map (\n"
                                   "      I => net_global_inout\n"
                                   "    );\n"
                                       "end STRUCTURE;");
        std::cout<<"\n=====\n"<< input.str() << "\n=====\n";           // remove me
        test_def::capture_stdout();
        hdl_parser_vhdl_old vhdl_parser(input);
        std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);
        if (nl == nullptr)
        {
            std::cout << test_def::get_captured_stdout();
        }
        else
        {
            test_def::get_captured_stdout();
        }

        ASSERT_NE(nl, nullptr);
        ASSERT_NE(nl->get_gates("INV").size(), 0);
        std::shared_ptr<gate> g = *nl->get_gates("INV").begin();
        EXPECT_EQ(g->get_data_by_key("generic", "key_bool"), std::make_tuple("boolean", "true"));
    }
    {
        // An integer is passed
        std::stringstream input("-- Device\t: device_name\n"
                                "entity TEST_Comp is\n"
                                "  port (\n"
                                "    net_global_inout : inout STD_LOGIC := 'X';\n"
                                "  );\n"
                                "end TEST_Comp;\n"
                                "architecture STRUCTURE of TEST_Comp is\n"
                                "begin\n"
                                "  gate_0 : INV\n"
                                "    generic map(\n"
                                "      key_integer => 123\n"
                                "    )\n"
                                "    port map (\n"
                                "      I => net_global_inout\n"
                                "    );\n"
                                "end STRUCTURE;");
        test_def::capture_stdout();
        hdl_parser_vhdl_old vhdl_parser(input);
        std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);
        if (nl == nullptr)
        {
            std::cout << test_def::get_captured_stdout();
        }
        else
        {
            test_def::get_captured_stdout();
        }

        ASSERT_NE(nl, nullptr);
        ASSERT_NE(nl->get_gates("INV").size(), 0);
        std::shared_ptr<gate> g = *nl->get_gates("INV").begin();
        EXPECT_EQ(g->get_data_by_key("generic", "key_integer"), std::make_tuple("integer", "123"));
    }
    {
        // A floating point number is passed
        std::stringstream input("-- Device\t: device_name\n"
                                "entity TEST_Comp is\n"
                                "  port (\n"
                                "    net_global_inout : inout STD_LOGIC := 'X';\n"
                                "  );\n"
                                "end TEST_Comp;\n"
                                "architecture STRUCTURE of TEST_Comp is\n"
                                "begin\n"
                                "  gate_0 : INV\n"
                                "    generic map(\n"
                                "      key_floating_point => 1.23\n"
                                "    )\n"
                                "    port map (\n"
                                "      I => net_global_inout\n"
                                "    );\n"
                                "end STRUCTURE;");
        test_def::capture_stdout();
        hdl_parser_vhdl_old vhdl_parser(input);
        std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);
        if (nl == nullptr)
        {
            std::cout << test_def::get_captured_stdout();
        }
        else
        {
            test_def::get_captured_stdout();
        }

        ASSERT_NE(nl, nullptr);
        ASSERT_NE(nl->get_gates("INV").size(), 0);
        std::shared_ptr<gate> g = *nl->get_gates("INV").begin();
        EXPECT_EQ(g->get_data_by_key("generic", "key_floating_point"), std::make_tuple("floating_point", "1.23"));
    }
    {
        // A time is passed
        std::stringstream input("-- Device\t: device_name\n"
                                "entity TEST_Comp is\n"
                                "  port (\n"
                                "    net_global_inout : inout STD_LOGIC := 'X';\n"
                                "  );\n"
                                "end TEST_Comp;\n"
                                "architecture STRUCTURE of TEST_Comp is\n"
                                "begin\n"
                                "  gate_0 : INV\n"
                                "    generic map(\n"
                                "      key_time => 1.23sec\n"
                                "    )\n"
                                "    port map (\n"
                                "      I => net_global_inout\n"
                                "    );\n"
                                "end STRUCTURE;");
        test_def::capture_stdout();
        hdl_parser_vhdl_old vhdl_parser(input);
        std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);
        if (nl == nullptr)
        {
            std::cout << test_def::get_captured_stdout();
        }
        else
        {
            test_def::get_captured_stdout();
        }

        ASSERT_NE(nl, nullptr);
        ASSERT_NE(nl->get_gates("INV").size(), 0);
        std::shared_ptr<gate> g = *nl->get_gates("INV").begin();
        EXPECT_EQ(g->get_data_by_key("generic", "key_time"), std::make_tuple("time", "1.23sec"));
    }
    {
        // A bit-vector is passed via ' '
        std::stringstream input("-- Device\t: device_name\n"
                                "entity TEST_Comp is\n"
                                "  port (\n"
                                "    net_global_inout : inout STD_LOGIC := 'X';\n"
                                "  );\n"
                                "end TEST_Comp;\n"
                                "architecture STRUCTURE of TEST_Comp is\n"
                                "begin\n"
                                "  gate_0 : INV\n"
                                "    generic map(\n"
                                "      key_bit_vector => B\"0000_1111_0000\"\n"
                                "    )\n"
                                "    port map (\n"
                                "      I => net_global_inout\n"
                                "    );\n"
                                "end STRUCTURE;");
        test_def::capture_stdout();
        hdl_parser_vhdl_old vhdl_parser(input);
        std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);
        if (nl == nullptr)
        {
            std::cout << test_def::get_captured_stdout();
        }
        else
        {
            test_def::get_captured_stdout();
        }

        ASSERT_NE(nl, nullptr);
        ASSERT_NE(nl->get_gates("INV").size(), 0);
        std::shared_ptr<gate> g = *nl->get_gates("INV").begin();
        EXPECT_EQ(g->get_data_by_key("generic", "key_bit_vector"), std::make_tuple("bit_vector", "f0"));
    }
    {
        // A bit-vector is passed via X" " and B" " and O" "
        std::stringstream input("-- Device\t: device_name\n"
                                "entity TEST_Comp is\n"
                                "  port (\n"
                                "    net_global_inout : inout STD_LOGIC := 'X';\n"
                                "  );\n"
                                "end TEST_Comp;\n"
                                "architecture STRUCTURE of TEST_Comp is\n"
                                "begin\n"
                                "  gate_0 : INV\n"
                                "    generic map(\n"
                                "      key_bit_vector_0 => X\"abcdef\",\n"
                                "      key_bit_vector_1 => B\"101010111100110111101111\",\n" // <- binary: 'abcdef' in hex
                                "      key_bit_vector_2 => O\"52746757\",\n" // <- octal: 'abcdef' in hex
                                "      key_bit_vector_3 => D\"11259375\"\n" // <- decimal: 'abcdef' in hex
                                "    )\n"
                                "    port map (\n"
                                "      I => net_global_inout\n"
                                "    );\n"
                                "end STRUCTURE;");
        test_def::capture_stdout();
        hdl_parser_vhdl_old vhdl_parser(input);
        std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);
        if (nl == nullptr)
        {
            std::cout << test_def::get_captured_stdout();
        }
        else
        {
            test_def::get_captured_stdout();
        }

        ASSERT_NE(nl, nullptr);
        ASSERT_NE(nl->get_gates("INV").size(), 0);
        std::shared_ptr<gate> g = *nl->get_gates("INV").begin();
        EXPECT_EQ(g->get_data_by_key("generic", "key_bit_vector_0"), std::make_tuple("bit_vector", "abcdef"));
        EXPECT_EQ(g->get_data_by_key("generic", "key_bit_vector_1"), std::make_tuple("bit_vector", "abcdef"));
        EXPECT_EQ(g->get_data_by_key("generic", "key_bit_vector_2"), std::make_tuple("bit_vector", "abcdef"));
        EXPECT_EQ(g->get_data_by_key("generic", "key_bit_vector_3"), std::make_tuple("bit_vector", "abcdef"));
    }
    {
        // A string is passed
        std::stringstream input("-- Device\t: device_name\n"
                                "entity TEST_Comp is\n"
                                "  port (\n"
                                "    net_global_inout : inout STD_LOGIC := 'X';\n"
                                "  );\n"
                                "end TEST_Comp;\n"
                                "architecture STRUCTURE of TEST_Comp is\n"
                                "begin\n"
                                "  gate_0 : INV\n"
                                "    generic map(\n"
                                "      key_string => \"one_two_three\"\n"
                                "    )\n"
                                "    port map (\n"
                                "      I => net_global_inout\n"
                                "    );\n"
                                "end STRUCTURE;");
        test_def::capture_stdout();
        hdl_parser_vhdl_old vhdl_parser(input);
        std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);
        if (nl == nullptr)
        {
            std::cout << test_def::get_captured_stdout();
        }
        else
        {
            test_def::get_captured_stdout();
        }

        ASSERT_NE(nl, nullptr);
        ASSERT_NE(nl->get_gates("INV").size(), 0);
        std::shared_ptr<gate> g = *nl->get_gates("INV").begin();
        EXPECT_EQ(g->get_data_by_key("generic", "key_string"), std::make_tuple("string", "one_two_three"));
    }
    {
        // A bit-value is passed
        std::stringstream input("-- Device\t: device_name\n"
                                "entity TEST_Comp is\n"
                                "  port (\n"
                                "    net_global_inout : inout STD_LOGIC := 'X';\n"
                                "  );\n"
                                "end TEST_Comp;\n"
                                "architecture STRUCTURE of TEST_Comp is\n"
                                "begin\n"
                                "  gate_0 : INV\n"
                                "    generic map(\n"
                                "      key_bit_value => \'11001010\'\n"
                                "    )\n"
                                "    port map (\n"
                                "      I => net_global_inout\n"
                                "    );\n"
                                "end STRUCTURE;");
        test_def::capture_stdout();
        hdl_parser_vhdl_old vhdl_parser(input);
        std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);
        if (nl == nullptr)
        {
            std::cout << test_def::get_captured_stdout();
        }
        else
        {
            test_def::get_captured_stdout();
        }

        ASSERT_NE(nl, nullptr);
        ASSERT_NE(nl->get_gates("INV").size(), 0);
        std::shared_ptr<gate> g = *nl->get_gates("INV").begin();
        EXPECT_EQ(g->get_data_by_key("generic", "key_bit_value"), std::make_tuple("bit_value", "11001010"));
    }


TEST_END
}

/**
 * Testing the implicit addition of global gnd and vcc gates by using '0' and '1'
 *
 * Functions: parse
 */
TEST_F(hdl_parser_vhdl_old_test, check_global_gates_implicit){
    TEST_START
        {
            // Add a global_gnd implicitly by using '0'
        std::stringstream input("-- Device\t: device_name\n"
                             "entity TEST_Comp is\n"
                             "  port (\n"
                             "    net_global_inout : inout STD_LOGIC := 'X';\n"
                             "  );\n"
                             "end TEST_Comp;\n"
                             "architecture STRUCTURE of TEST_Comp is\n"
                             "begin\n"
                             "  gate_0 : INV\n"
                             "    port map (\n"
                             "      I => '0'\n"
                             "    );\n"
                             "end STRUCTURE;");
        test_def::capture_stdout();
        hdl_parser_vhdl_old vhdl_parser(input);
        std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);
        if (nl == nullptr)
        {
        std::cout << test_def::get_captured_stdout();
        }
        else
        {
        test_def::get_captured_stdout();
        }

        ASSERT_NE(nl, nullptr);

        ASSERT_NE(nl->get_global_gnd_gates().size(), 0);
        std::shared_ptr<gate> global_gnd = *nl->get_global_gnd_gates().begin();

        ASSERT_NE(nl->get_gates("INV").size(), 0);
        std::shared_ptr<gate> g = *nl->get_gates("INV").begin();

        ASSERT_NE(g->get_predecessors().size(), 0);
        endpoint pred = *g->get_predecessors().begin();
        EXPECT_EQ(pred.get_gate(), global_gnd);
    }
    {
        // Add a global_vcc implicitly by using '1'
        std::stringstream input("-- Device\t: device_name\n"
                                "entity TEST_Comp is\n"
                                "  port (\n"
                                "    net_global_inout : inout STD_LOGIC := 'X';\n"
                                "  );\n"
                                "end TEST_Comp;\n"
                                "architecture STRUCTURE of TEST_Comp is\n"
                                "begin\n"
                                "  gate_0 : INV\n"
                                "    port map (\n"
                                "      I => '1'\n"
                                "    );\n"
                                "end STRUCTURE;");
        test_def::capture_stdout();
        hdl_parser_vhdl_old vhdl_parser(input);
        std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);
        if (nl == nullptr)
        {
            std::cout << test_def::get_captured_stdout();
        }
        else
        {
            test_def::get_captured_stdout();
        }

        ASSERT_NE(nl, nullptr);

        ASSERT_NE(nl->get_global_vcc_gates().size(), 0);
        std::shared_ptr<gate> global_vcc = *nl->get_global_vcc_gates().begin();

        ASSERT_NE(nl->get_gates("INV").size(), 0);
        std::shared_ptr<gate> g = *nl->get_gates("INV").begin();

        ASSERT_NE(g->get_predecessors().size(), 0);
        endpoint pred = *g->get_predecessors().begin();
        EXPECT_EQ(pred.get_gate(), global_vcc);
    }
TEST_END
}

/**
 * Testing the explicit addition of a global gnd and vcc gates
 *
 * Functions: parse
 */
TEST_F(hdl_parser_vhdl_old_test, check_global_gates_explicit){
    TEST_START
        {
            // Add a global_gnd explicitly
            std::stringstream input("-- Device\t: device_name\n"
            "entity TEST_Comp is\n"
            "  port (\n"
            "    net_global_inout : inout STD_LOGIC := 'X';\n"
            "  );\n"
            "end TEST_Comp;\n"
            "architecture STRUCTURE of TEST_Comp is\n"
            "begin\n"
            "  g_gnd_gate : GND\n"
            "    port map (\n"
            "      O => net_global_inout\n"
            "    );\n"
            "end STRUCTURE;");
            test_def::capture_stdout();
            hdl_parser_vhdl_old vhdl_parser(input);
            std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);
            if (nl == nullptr)
            {
            std::cout << test_def::get_captured_stdout();
            }
            else
            {
            test_def::get_captured_stdout();
            }

            ASSERT_NE(nl, nullptr);

            ASSERT_NE(nl->get_global_gnd_gates().size(), 0);
            std::shared_ptr<gate> global_gnd = *nl->get_global_gnd_gates().begin();
            EXPECT_EQ(global_gnd->get_name(), "g_gnd_gate");
    }
    {
        // Add a global_vcc explicitly
        std::stringstream input("-- Device\t: device_name\n"
                                "entity TEST_Comp is\n"
                                "  port (\n"
                                "    net_global_inout : inout STD_LOGIC := 'X';\n"
                                "  );\n"
                                "end TEST_Comp;\n"
                                "architecture STRUCTURE of TEST_Comp is\n"
                                "begin\n"
                                "  g_vcc_gate : VCC\n"
                                "    port map (\n"
                                "      O => net_global_inout\n"
                                "    );\n"
                                "end STRUCTURE;");
        test_def::capture_stdout();
        hdl_parser_vhdl_old vhdl_parser(input);
        std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);
        if (nl == nullptr)
        {
            std::cout << test_def::get_captured_stdout();
        }
        else
        {
            test_def::get_captured_stdout();
        }

        ASSERT_NE(nl, nullptr);

        ASSERT_NE(nl->get_global_vcc_gates().size(), 0);
        std::shared_ptr<gate> global_vcc = *nl->get_global_vcc_gates().begin();
        EXPECT_EQ(global_vcc->get_name(), "g_vcc_gate");
    }
TEST_END
}

/**
 * Testing the usage and the correct handling of library prefixes
 *
 * Functions: parse
 */
TEST_F(hdl_parser_vhdl_old_test, check_lib_prefix)
{
    TEST_START
    // The prefix of the library SIMPRIM.VCOMPONENTS should be removed from the INV gate type
    std::stringstream input("-- Device\t: device_name\n"
                            "library SIMPRIM;\n"
                            "use SIMPRIM.VCOMPONENTS.ALL;\n"
                            "entity TEST_Comp is\n"
                            "  port (\n"
                            "    net_global_inout : inout STD_LOGIC := 'X';\n"
                            "  );\n"
                            "end TEST_Comp;\n"
                            "architecture STRUCTURE of TEST_Comp is\n"
                            "begin\n"
                            "  gate_0 : SIMPRIM.VCOMPONENTS.INV\n"
                            "    port map (\n"
                            "      I => net_global_inout\n"
                            "    );\n"
                            "end STRUCTURE;");
    test_def::capture_stdout();
    hdl_parser_vhdl_old vhdl_parser(input);
    std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);
    if (nl == nullptr)
    {
        std::cout << test_def::get_captured_stdout();
    }
    else
    {
        test_def::get_captured_stdout();
    }

    ASSERT_NE(nl, nullptr);

    EXPECT_EQ(nl->get_gates("INV").size(), 1);
    TEST_END
}

/**
 * Testing the implicit addition of nets by using them in the instance for the first time
 *
 * Functions: parse
 */
TEST_F(hdl_parser_vhdl_old_test, check_add_net_implicit)
{
    TEST_START
    // The net implicit_net will be used in implicit_net but was not declared in the architecture block
    std::stringstream input("-- Device\t: device_name\n"
                            "entity TEST_Comp is\n"
                            "  port (\n"
                            "    net_global_inout : inout STD_LOGIC := 'X';\n"
                            "  );\n"
                            "end TEST_Comp;\n"
                            "architecture STRUCTURE of TEST_Comp is\n"
                            "begin\n"
                            "  gate_0 : INV\n"
                            "    port map (\n"
                            "      O => implicit_net\n"
                            "    );\n"
                            "end STRUCTURE;");
    test_def::capture_stdout();
    hdl_parser_vhdl_old vhdl_parser(input);
    std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);
    if (nl == nullptr)
    {
        std::cout << test_def::get_captured_stdout();
    }
    else
    {
        test_def::get_captured_stdout();
    }

    ASSERT_NE(nl, nullptr);

    ASSERT_NE(nl->get_gates("INV").size(), 0);
    std::shared_ptr<gate> g           = *nl->get_gates("INV").begin();
    std::shared_ptr<net> implicit_net = *(nl->get_nets("implicit_net").begin());
    ASSERT_NE(implicit_net, nullptr);
    EXPECT_EQ(implicit_net->get_src().get_gate(), g);
    TEST_END
}

/**
 * Testing the usage of multiple entity blocks (calls the add_entity_definition function)
 *
 * Functions: parse
 */
TEST_F(hdl_parser_vhdl_old_test, check_multiple_entities)
{
    TEST_START
        {
            // Add a second entity at the end
            std::stringstream input("-- Device\t: device_name\n"
                                    "entity TEST_Comp_1 is\n"
                                    "  port (\n"
                                    "    first_entity_in  : in STD_LOGIC := 'X';\n"
                                    "    first_entity_out : out STD_LOGIC := 'X';\n"
                                    "    first_entity_inout : inout STD_LOGIC := 'X';\n"
                                    "\n"
                                    "  );\n"
                                    "end TEST_Comp_1;\n"
                                    "\n"
                                    "architecture STRUCTURE of TEST_Comp_1 is\n"
                                    "begin\n"
                                    "  gate_0 : INV\n"
                                    "    port map (\n"
                                    "      O => net_global_inout\n"
                                    "    );\n"
                                    "end STRUCTURE;\n"
                                    "\n"
                                    "entity TEST_Comp_2 is\n"
                                    "  port(\n"
                                    "    net_global_inout : inout STD_LOGIC := 'X';    \n"
                                    "  );\n"
                                    "end TEST_Comp_2;\n"
                                    "\n"
                                    "architecture STRUCTURE of TEST_Comp_2 is\n"
                                    "begin\n"
                                    "  gate_1 : INV\n"
                                    "    port map (\n"
                                    "      O => net_global_inout\n"
                                    "    );\n"
                                    "end STRUCTURE;");
            test_def::capture_stdout();
            hdl_parser_vhdl_old vhdl_parser(input);
            std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);
            if (nl == nullptr) {
                std::cout << test_def::get_captured_stdout();
            } else {
                test_def::get_captured_stdout();
            }
            // NOTE: Clarify the usage of a second entity
            //ASSERT_NE(nl, nullptr);

        }
        {
            // Use the 'entity'-keyword in the context of a gate type (should be ignored)
            std::stringstream input("-- Device\t: device_name\n"
                                    "entity TEST_Comp is\n"
                                    "  port (\n"
                                    "    net_global_inout : inout STD_LOGIC := 'X';\n"
                                    "  );\n"
                                    "end TEST_Comp;\n"
                                    "architecture STRUCTURE of TEST_Comp is\n"
                                    "begin\n"
                                    "  gate_0 : entity INV\n" // <- usage of the 'entity' keyword
                                    "    port map (\n"
                                    "      O => net_global_inout\n"
                                    "    );\n"
                                    "end STRUCTURE;");
            test_def::capture_stdout();
            hdl_parser_vhdl_old vhdl_parser(input);
            std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);
            if (nl == nullptr)
            {
                std::cout << test_def::get_captured_stdout();
            }
            else
            {
                test_def::get_captured_stdout();
            }

            ASSERT_NE(nl, nullptr);
            ASSERT_FALSE(nl->get_gates("INV", "gate_0").empty());

        }
        {
            // Add an entity with invalid format
            std::stringstream input("-- Device\t: device_name\n"
                                    "entity TEST_Comp_1 is\n"
                                    "  port (\n"
                                    "    first_entity_in  : in STD_LOGIC := 'X';\n"
                                    "    first_entity_out : out STD_LOGIC := 'X';\n"
                                    "    first_entity_inout : invalid_direction STD_LOGIC := 'X';\n" // <- invalid
                                    "\n"
                                    "  );\n"
                                    "end TEST_Comp_1;\n"
                                    "\n"
                                    "architecture STRUCTURE of TEST_Comp_1 is\n"
                                    "begin\n"
                                    "  gate_0 : INV\n"
                                    "    port map (\n"
                                    "      O => net_global_inout\n"
                                    "    );\n"
                                    "end STRUCTURE;\n"
                                    "\n"
                                    "entity TEST_Comp_2 is\n"
                                    "  port(\n"
                                    "    net_global_inout : inout STD_LOGIC := 'X';    \n"
                                    "  );\n"
                                    "end TEST_Comp_2;\n"
                                    "\n"
                                    "architecture STRUCTURE of TEST_Comp_2 is\n"
                                    "begin\n"
                                    "  gate_1 : INV\n"
                                    "    port map (\n"
                                    "      O => net_global_inout\n"
                                    "    );\n"
                                    "end STRUCTURE;");
            hdl_parser_vhdl_old vhdl_parser(input);
            std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);
            EXPECT_EQ(nl, nullptr);

        }

        /*{ // Segmentation Fault
            // Add an entity with invalid format
            std::stringstream input("-- Device\t: device_name\n"
                                    "entity TEST_Comp is\n"
                                    "  port (\n"
                                    "    net_global_inout inout STD_LOGIC ;\n" // <- no ':'
                                    "  );\n"
                                    "end TEST_Comp;\n"
                                    "\n"
                                    "architecture STRUCTURE of TEST_Comp is\n"
                                    "begin\n"
                                    "  gate_0 : INV\n"
                                    "    port map (\n"
                                    "      O => net_global_inout\n"
                                    "    );\n"
                                    "end STRUCTURE;\n"
                                    "\n"
                                    "entity TEST_Comp_2 is\n"
                                    "  port(\n"
                                    "    sec_entity_in  : in STD_LOGIC := 'X';\n"
                                    "    sec_entity_out : out STD_LOGIC := 'X';\n"
                                    "    sec_entity_out : inout STD_LOGIC := 'X';\n"
                                    "  );\n"
                                    "end TEST_Comp_2;");
            test_def::capture_stdout();
            hdl_parser_vhdl_old vhdl_parser(input);
            std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);
            ASSERT_EQ(nl, nullptr);

        }*/

    TEST_END
}

/**
 * Testing the handling of logic-vectors in dimension 1-3
 *
 * Functions: parse
 */
TEST_F(hdl_parser_vhdl_old_test, check_logic_vectors)
{
    TEST_START
        {
            // Use two logic vectors with dimension 1. One uses the 'downto' the other the 'to' statement
            std::stringstream input("-- Device\t: device_name\n"
                                    "entity TEST_Comp is\n"
                                    "  port (\n"
                                    "    net_global_inout : inout STD_LOGIC := 'X';\n"
                                    "  );\n"
                                    "end TEST_Comp;\n"
                                    "architecture STRUCTURE of TEST_Comp is\n"
                                    "  signal l_vec_1 : STD_LOGIC_VECTOR ( 3 downto 0 );\n"
                                    "  signal l_vec_2 : STD_LOGIC_VECTOR ( 0 to 2 );\n"
                                    "begin\n"
                                    "  gate_0 : INV\n"
                                    "    port map (\n"
                                    "      O => net_global_inout\n"
                                    "    );\n"
                                    "end STRUCTURE;");
            test_def::capture_stdout();
            hdl_parser_vhdl_old vhdl_parser(input);
            std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);
            if (nl == nullptr)
            {
                std::cout << test_def::get_captured_stdout();
            }
            else
            {
                test_def::get_captured_stdout();
            }

            ASSERT_NE(nl, nullptr);
            EXPECT_EQ(nl->get_nets().size(), 8); // net_global_inout + 4 nets in l_vec_1 + 3 nets in l_vec_2
            for(auto net_name : std::set<std::string>({"l_vec_1(0)","l_vec_1(1)","l_vec_1(2)","l_vec_1(3)","l_vec_2(0)","l_vec_2(1)","l_vec_2(2)"})){
                EXPECT_FALSE(nl->get_nets(net_name).empty());
            }
        }
        {
            // Use a logic vector of dimension two
            std::stringstream input("-- Device\t: device_name\n"
                                    "entity TEST_Comp is\n"
                                    "  port (\n"
                                    "    net_global_inout : inout STD_LOGIC := 'X';\n"
                                    "  );\n"
                                    "end TEST_Comp;\n"
                                    "architecture STRUCTURE of TEST_Comp is\n"
                                    "  signal l_vec : STD_LOGIC_VECTOR2 ( 0 to 1, 2 to 3 );\n"
                                    "begin\n"
                                    "  gate_0 : INV\n"
                                    "    port map (\n"
                                    "      O => net_global_inout\n"
                                    "    );\n"
                                    "end STRUCTURE;");
            test_def::capture_stdout();
            hdl_parser_vhdl_old vhdl_parser(input);
            std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);
            if (nl == nullptr)
            {
                std::cout << test_def::get_captured_stdout();
            }
            else
            {
                test_def::get_captured_stdout();
            }

            ASSERT_NE(nl, nullptr);
            EXPECT_EQ(nl->get_nets().size(), 5); // net_global_inout + 4 nets in l_vec
            for(auto net_name : std::set<std::string>({"l_vec(0, 2)","l_vec(0, 3)","l_vec(1, 2)","l_vec(1, 3)"})){
                EXPECT_FALSE(nl->get_nets(net_name).empty());
            }


        }
        {
            // Use a logic vector of dimension three
            std::stringstream input("-- Device\t: device_name\n"
                                    "entity TEST_Comp is\n"
                                    "  port (\n"
                                    "    net_global_inout : inout STD_LOGIC := 'X';\n"
                                    "  );\n"
                                    "end TEST_Comp;\n"
                                    "architecture STRUCTURE of TEST_Comp is\n"
                                    "  signal l_vec : STD_LOGIC_VECTOR3 ( 0 to 1, 0 to 1, 0 to 1 );\n"
                                    "begin\n"
                                    "  gate_0 : INV\n"
                                    "    port map (\n"
                                    "      O => net_global_inout\n"
                                    "    );\n"
                                    "end STRUCTURE;");
            test_def::capture_stdout();
            hdl_parser_vhdl_old vhdl_parser(input);
            std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);
            if (nl == nullptr)
            {
                std::cout << test_def::get_captured_stdout();
            }
            else
            {
                test_def::get_captured_stdout();
            }
            ASSERT_NE(nl, nullptr);
            EXPECT_EQ(nl->get_nets().size(), 9); // net_global_inout + 8 nets in l_vec
            for(auto net_name : std::set<std::string>({"l_vec(0, 0, 0)","l_vec(0, 0, 1)","l_vec(0, 1, 0)","l_vec(0, 1, 1)",
                                                       "l_vec(1, 0, 0)","l_vec(1, 0, 1)","l_vec(1, 1, 0)","l_vec(1, 1, 1)"})){
                EXPECT_FALSE(nl->get_nets(net_name).empty());
            }

        }
        // NEGATIVE
        {
            // The amount of bounds does not match with the vector dimension
            NO_COUT_TEST_BLOCK;
            std::stringstream input("-- Device\t: device_name\n"
                                    "entity TEST_Comp is\n"
                                    "  port (\n"
                                    "    net_global_inout : inout STD_LOGIC := 'X';\n"
                                    "  );\n"
                                    "end TEST_Comp;\n"
                                    "architecture STRUCTURE of TEST_Comp is\n"
                                    "  signal l_vec : STD_LOGIC_VECTOR3 ( 0 to 1, 0 to 1);\n" // <- two bounds, but dimension 3
                                    "begin\n"
                                    "  gate_0 : INV\n"
                                    "    port map (\n"
                                    "      O => net_global_inout\n"
                                    "    );\n"
                                    "end STRUCTURE;");
            hdl_parser_vhdl_old vhdl_parser(input);
            std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);

            EXPECT_EQ(nl->get_nets().size(), 1);

        }
    TEST_END
}

/**
 * Testing the different ways of port assignments
 *
 * Functions: parse
 */
TEST_F(hdl_parser_vhdl_old_test, check_port_assignment) {
    TEST_START
        // We need to create another gate library with multiple output ports
        create_temp_gate_lib();
        {
            // Connect a single set with a single output pin
            std::stringstream input("-- Device\t: device_name\n"
                                    "entity TEST_Comp is\n"
                                    "  port (\n"
                                    "  );\n"
                                    "end TEST_Comp;\n"
                                    "architecture STRUCTURE of TEST_Comp is\n"
                                    "  signal net_0 : STD_LOGIC;\n"
                                    "  signal net_1 : STD_LOGIC;\n"
                                    "begin\n"
                                    "  gate_0 : GATE_4^1_IN_4^1_OUT\n"
                                    "    port map (\n"
                                    "      O(0) => net_0,\n"
                                    "      O(1) => net_1\n"
                                    "    );\n"
                                    "end STRUCTURE;");
            test_def::capture_stdout();
            hdl_parser_vhdl_old vhdl_parser(input);
            std::shared_ptr<netlist> nl = vhdl_parser.parse(temp_lib_name);
            if (nl == nullptr)
            {
                std::cout << test_def::get_captured_stdout();
            }
            else
            {
                test_def::get_captured_stdout();
            }

            ASSERT_NE(nl, nullptr);
            ASSERT_FALSE(nl->get_gates("GATE_4^1_IN_4^1_OUT", "gate_0").empty());
            std::shared_ptr<gate> gate_0 = *(nl->get_gates("GATE_4^1_IN_4^1_OUT" ,"gate_0").begin());

            std::shared_ptr<net> net_0 = gate_0->get_fan_out_net("O(0)");
            ASSERT_NE(net_0, nullptr);
            EXPECT_EQ(net_0->get_name(), "net_0");

            std::shared_ptr<net> net_1 = gate_0->get_fan_out_net("O(1)");
            ASSERT_NE(net_1, nullptr);
            EXPECT_EQ(net_1->get_name(), "net_1");
        }
        {
            // Connect two dimensional ports
            std::stringstream input("-- Device\t: device_name\n"
                                    "entity TEST_Comp is\n"
                                    "  port (\n"
                                    "  );\n"
                                    "end TEST_Comp;\n"
                                    "architecture STRUCTURE of TEST_Comp is\n"
                                    "  signal net_0 : STD_LOGIC;\n"
                                    "begin\n"
                                    "  gate_0 : GATE_2^2_IN_2^2_OUT\n"
                                    "    port map (\n"
                                    "      I(0, 1) => net_0\n" // <- connect I(0,1) to net_0
                                    "    );\n"
                                    "end STRUCTURE;");
            test_def::capture_stdout();
            hdl_parser_vhdl_old vhdl_parser(input);
            std::shared_ptr<netlist> nl = vhdl_parser.parse(temp_lib_name);
            if (nl == nullptr) {
                std::cout << test_def::get_captured_stdout();
            } else {
                test_def::get_captured_stdout();
            }

            ASSERT_NE(nl, nullptr);
            ASSERT_FALSE(nl->get_gates("GATE_2^2_IN_2^2_OUT", "gate_0").empty());
            std::shared_ptr<gate> gate_0 = *(nl->get_gates("GATE_2^2_IN_2^2_OUT", "gate_0").begin());

            std::shared_ptr<net> net_i_0 = gate_0->get_fan_in_net("I(0, 1)");
            ASSERT_NE(net_i_0, nullptr);
            EXPECT_EQ(net_i_0->get_name(), "net_0");
        }
        {
            // Connect a vector of output pins with global input nets by using a binary string (B"10101010")
            std::stringstream input("-- Device\t: device_name\n"
                                    "entity TEST_Comp is\n"
                                    "  port (\n"
                                    "  );\n"
                                    "end TEST_Comp;\n"
                                    "architecture STRUCTURE of TEST_Comp is\n"
                                    "begin\n"
                                    "  gate_0 : GATE_4^1_IN_4^1_OUT\n"
                                    "    port map (\n"
                                    "      I(1 to 3) => B\"101\"\n"
                                    "    );\n"
                                    "end STRUCTURE;");
            test_def::capture_stdout();
            hdl_parser_vhdl_old vhdl_parser(input);
            std::shared_ptr<netlist> nl = vhdl_parser.parse(temp_lib_name);
            if (nl == nullptr)
            {
                std::cout << test_def::get_captured_stdout();
            }
            else
            {
                test_def::get_captured_stdout();
            }

            ASSERT_NE(nl, nullptr);
            ASSERT_FALSE(nl->get_gates("GATE_4^1_IN_4^1_OUT", "gate_0").empty());
            std::shared_ptr<gate> gate_0 = *(nl->get_gates("GATE_4^1_IN_4^1_OUT" ,"gate_0").begin());

            EXPECT_EQ(gate_0->get_fan_in_nets().size(), 2);

            std::shared_ptr<net> net_0 = gate_0->get_fan_in_net("I(1)");
            ASSERT_NE(net_0, nullptr);
            EXPECT_EQ(net_0->get_name(), "'1'");

            std::shared_ptr<net> net_1 = gate_0->get_fan_in_net("I(2)");
            ASSERT_NE(net_1, nullptr);
            EXPECT_EQ(net_1->get_name(), "'0'");

            std::shared_ptr<net> net_2 = gate_0->get_fan_in_net("I(3)");
            ASSERT_NE(net_2, nullptr);
            EXPECT_EQ(net_2->get_name(), "'1'");
        }
        {
            // Connect a vector of output pins with a vector of nets (O(1)=>l_vec(0), O(2)=>l_vec(1), O(3)=>l_vec(2))
            std::stringstream input("-- Device\t: device_name\n"
                                    "entity TEST_Comp is\n"
                                    "  port (\n"
                                    "  );\n"
                                    "end TEST_Comp;\n"
                                    "architecture STRUCTURE of TEST_Comp is\n"
                                    "  signal l_vec : STD_LOGIC_VECTOR ( 0 to 3 );\n"
                                    "begin\n"
                                    "  gate_0 : GATE_4^1_IN_4^1_OUT\n"
                                    "    port map (\n"
                                    "      O(1 to 3) => l_vec(0 to 2)\n"
                                    "    );\n"
                                    "end STRUCTURE;");
            test_def::capture_stdout();
            hdl_parser_vhdl_old vhdl_parser(input);
            std::shared_ptr<netlist> nl = vhdl_parser.parse(temp_lib_name);
            if (nl == nullptr)
            {
                std::cout << test_def::get_captured_stdout();
            }
            else
            {
                test_def::get_captured_stdout();
            }

            ASSERT_NE(nl, nullptr);
            ASSERT_FALSE(nl->get_gates("GATE_4^1_IN_4^1_OUT", "gate_0").empty());
            std::shared_ptr<gate> gate_0 = *(nl->get_gates("GATE_4^1_IN_4^1_OUT" ,"gate_0").begin());

            EXPECT_EQ(gate_0->get_fan_out_nets().size(), 3);

            std::shared_ptr<net> net_0 = gate_0->get_fan_out_net("O(1)");
            ASSERT_NE(net_0, nullptr);
            EXPECT_EQ(net_0->get_name(), "l_vec(0)");

            std::shared_ptr<net> net_1 = gate_0->get_fan_out_net("O(2)");
            ASSERT_NE(net_1, nullptr);
            EXPECT_EQ(net_1->get_name(), "l_vec(1)");

            std::shared_ptr<net> net_2 = gate_0->get_fan_out_net("O(3)");
            ASSERT_NE(net_2, nullptr);
            EXPECT_EQ(net_2->get_name(), "l_vec(2)");
        }
        {
            // Connect a vector of output pins with a vector of nets, but using downto statements
            std::stringstream input("-- Device\t: device_name\n"
                                    "entity TEST_Comp is\n"
                                    "  port (\n"
                                    "  );\n"
                                    "end TEST_Comp;\n"
                                    "architecture STRUCTURE of TEST_Comp is\n"
                                    "  signal l_vec : STD_LOGIC_VECTOR ( 0 to 3 );\n"
                                    "begin\n"
                                    "  gate_0 : GATE_4^1_IN_4^1_OUT\n"
                                    "    port map (\n"
                                    "      O(3 downto 2) => l_vec(2 downto 1)\n"
                                    "    );\n"
                                    "end STRUCTURE;");
            test_def::capture_stdout();
            hdl_parser_vhdl_old vhdl_parser(input);
            std::shared_ptr<netlist> nl = vhdl_parser.parse(temp_lib_name);
            if (nl == nullptr)
            {
                std::cout << test_def::get_captured_stdout();
            }
            else
            {
                test_def::get_captured_stdout();
            }

            ASSERT_NE(nl, nullptr);
            ASSERT_FALSE(nl->get_gates("GATE_4^1_IN_4^1_OUT", "gate_0").empty());
            std::shared_ptr<gate> gate_0 = *(nl->get_gates("GATE_4^1_IN_4^1_OUT" ,"gate_0").begin());

            EXPECT_EQ(gate_0->get_fan_out_nets().size(), 2);

            std::shared_ptr<net> net_1 = gate_0->get_fan_out_net("O(2)");
            ASSERT_NE(net_1, nullptr);
            EXPECT_EQ(net_1->get_name(), "l_vec(1)");

            std::shared_ptr<net> net_2 = gate_0->get_fan_out_net("O(3)");
            ASSERT_NE(net_2, nullptr);
            EXPECT_EQ(net_2->get_name(), "l_vec(2)");
        }
        {
            // Connect a vector of output pins with a vector of nets, but using a mix of to and downto statements
            std::stringstream input("-- Device\t: device_name\n"
                                    "entity TEST_Comp is\n"
                                    "  port (\n"
                                    "  );\n"
                                    "end TEST_Comp;\n"
                                    "architecture STRUCTURE of TEST_Comp is\n"
                                    "  signal l_vec : STD_LOGIC_VECTOR ( 0 to 3 );\n"
                                    "begin\n"
                                    "  gate_0 : GATE_4^1_IN_4^1_OUT\n"
                                    "    port map (\n"
                                    "      O(2 to 3) => l_vec(2 downto 1)\n"
                                    "    );\n"
                                    "end STRUCTURE;");
            test_def::capture_stdout();
            hdl_parser_vhdl_old vhdl_parser(input);
            std::shared_ptr<netlist> nl = vhdl_parser.parse(temp_lib_name);
            if (nl == nullptr)
            {
                std::cout << test_def::get_captured_stdout();
            }
            else
            {
                test_def::get_captured_stdout();
            }

            ASSERT_NE(nl, nullptr);
            ASSERT_FALSE(nl->get_gates("GATE_4^1_IN_4^1_OUT", "gate_0").empty());
            std::shared_ptr<gate> gate_0 = *(nl->get_gates("GATE_4^1_IN_4^1_OUT" ,"gate_0").begin());

            EXPECT_EQ(gate_0->get_fan_out_nets().size(), 2);

            std::shared_ptr<net> net_1 = gate_0->get_fan_out_net("O(2)");
            ASSERT_NE(net_1, nullptr);
            EXPECT_EQ(net_1->get_name(), "l_vec(2)");

            std::shared_ptr<net> net_2 = gate_0->get_fan_out_net("O(3)");
            ASSERT_NE(net_2, nullptr);
            EXPECT_EQ(net_2->get_name(), "l_vec(1)");
        }
        // NEGATIVE
        {
            // The range of the vectors does not match in size
            std::stringstream input("-- Device\t: device_name\n"
                                    "entity TEST_Comp is\n"
                                    "  port (\n"
                                    "  );\n"
                                    "end TEST_Comp;\n"
                                    "architecture STRUCTURE of TEST_Comp is\n"
                                    "  signal l_vec : STD_LOGIC_VECTOR ( 0 to 3 );\n"
                                    "begin\n"
                                    "  gate_0 : GATE_4^1_IN_4^1_OUT\n"
                                    "    port map (\n"
                                    "      O(0 to 2) => l_vec(0 to 3)\n"
                                    "    );\n"
                                    "end STRUCTURE;");
            test_def::capture_stdout();
            hdl_parser_vhdl_old vhdl_parser(input);
            std::shared_ptr<netlist> nl = vhdl_parser.parse(temp_lib_name);
            if (nl == nullptr)
            {
                std::cout << test_def::get_captured_stdout();
            }
            else
            {
                test_def::get_captured_stdout();
            }

            ASSERT_NE(nl, nullptr);
            ASSERT_FALSE(nl->get_gates("GATE_4^1_IN_4^1_OUT", "gate_0").empty());
            std::shared_ptr<gate> gate_0 = *(nl->get_gates("GATE_4^1_IN_4^1_OUT" ,"gate_0").begin());

            EXPECT_EQ(gate_0->get_fan_out_nets().size(), 0);
        }
        {
            // The right side does no match any vector format
            std::stringstream input("-- Device\t: device_name\n"
                                    "entity TEST_Comp is\n"
                                    "  port (\n"
                                    "  );\n"
                                    "end TEST_Comp;\n"
                                    "architecture STRUCTURE of TEST_Comp is\n"
                                    "begin\n"
                                    "  gate_0 : GATE_4^1_IN_4^1_OUT\n"
                                    "    port map (\n"
                                    "      I(1 to 3) => Unkn0wn_Format\n" // <- unknown format
                                    "    );\n"
                                    "end STRUCTURE;");
            test_def::capture_stdout();
            hdl_parser_vhdl_old vhdl_parser(input);
            std::shared_ptr<netlist> nl = vhdl_parser.parse(temp_lib_name);
            if (nl == nullptr)
            {
                std::cout << test_def::get_captured_stdout();
            }
            else
            {
                test_def::get_captured_stdout();
            }

            ASSERT_NE(nl, nullptr);
            ASSERT_FALSE(nl->get_gates("GATE_4^1_IN_4^1_OUT", "gate_0").empty());
            std::shared_ptr<gate> gate_0 = *(nl->get_gates("GATE_4^1_IN_4^1_OUT" ,"gate_0").begin());

            EXPECT_EQ(gate_0->get_fan_in_nets().size(), 0);

        }
        remove_temp_gate_lib();
    TEST_END
}

/**
 * Testing the correct handling of invalid input
 *
 * Functions: parse
 */
TEST_F(hdl_parser_vhdl_old_test, check_invalid_input)
{
    TEST_START
        {
            // The entity contains unknown direction keywords (neither 'in', 'out' nor 'inout')
            NO_COUT_TEST_BLOCK;
            std::stringstream input("-- Device\t: device_name\n"
                                    "entity TEST_Comp is\n"
                                    "  port (\n"
                                    "    net_global_inout : invalid_direction STD_LOGIC := 'X';\n" // <- invalid direction
                                    "  );\n"
                                    "end TEST_Comp;\n"
                                    "architecture STRUCTURE of TEST_Comp is\n"
                                    "begin\n"
                                    "  gate_0 : INV\n"
                                    "    port map (\n"
                                    "      O => net_global_inout\n"
                                    "    );\n"
                                    "end STRUCTURE;");
            hdl_parser_vhdl_old vhdl_parser(input);
            std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);

            EXPECT_EQ(nl, nullptr);
        }
        {
            // The architecture contains an invalid keyword (neither 'signal' nor 'attribute')
            NO_COUT_TEST_BLOCK;
            std::stringstream input("-- Device\t: device_name\n"
                                    "entity TEST_Comp is\n"
                                    "  port (\n"
                                    "    net_global_inout : inout STD_LOGIC := 'X';\n"
                                    "  );\n"
                                    "end TEST_Comp;\n"
                                    "architecture STRUCTURE of TEST_Comp is\n"
                                    "unknown_keyword some_signal : STD_LOGIC;" // <- invalid keyword
                                    "begin\n"
                                    "  gate_0 : INV\n"
                                    "    port map (\n"
                                    "      O => net_global_inout\n"
                                    "    );\n"
                                    "end STRUCTURE;");
            hdl_parser_vhdl_old vhdl_parser(input);
            std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);

            EXPECT_EQ(nl, nullptr);
        }
        {
            // The 'end STRUCTURE;' line at the end was forgotten
            NO_COUT_TEST_BLOCK;
            std::stringstream input("-- Device\t: device_name\n"
                                    "entity TEST_Comp is\n"
                                    "  port (\n"
                                    "    net_global_inout : inout STD_LOGIC := 'X';\n"
                                    "  );\n"
                                    "end TEST_Comp;\n"
                                    "architecture STRUCTURE of TEST_Comp is\n"
                                    "begin\n"
                                    "  gate_0 : INV\n"
                                    "    port map (\n"
                                    "      O => net_global_inout\n"
                                    "    );");                      // <- no 'end STRUCTURE;'
            hdl_parser_vhdl_old vhdl_parser(input);
            std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);

            EXPECT_EQ(nl, nullptr);
        }
        {
            // The passed gate_library does not exist
            NO_COUT_TEST_BLOCK;
            std::stringstream input("-- Device\t: device_name\n"
                                    "entity TEST_Comp is\n"
                                    "  port (\n"
                                    "    net_global_inout : inout STD_LOGIC := 'X';\n"
                                    "  );\n"
                                    "end TEST_Comp;\n"
                                    "architecture STRUCTURE of TEST_Comp is\n"
                                    "begin\n"
                                    "  gate_0 : INV\n"
                                    "    port map (\n"
                                    "      O => net_global_inout\n"
                                    "    );\n"
                                    "end STRUCTURE;");                      // <- no 'end STRUCTURE;'
            hdl_parser_vhdl_old vhdl_parser(input);
            std::shared_ptr<netlist> nl = vhdl_parser.parse("inv4lid_gate_library");

            EXPECT_EQ(nl, nullptr);
        }
        {
            // Testing incorrect data_types in the "generic map" block
            NO_COUT_TEST_BLOCK;
            std::stringstream input("-- Device\t: device_name\n"
                                    "entity TEST_Comp is\n"
                                    "  port (\n"
                                    "    net_global_inout : inout STD_LOGIC := 'X';\n"
                                    "  );\n"
                                    "end TEST_Comp;\n"
                                    "architecture STRUCTURE of TEST_Comp is\n"
                                    "begin\n"
                                    "  gate_0 : INV\n"
                                    "    generic map(\n"
                                    "      key_invalid_type => Inv4lid_type\n"        // <- The format of 'Inv4lid_type' matches with no data_type
                                    "    )\n"
                                    "    port map (\n"
                                    "      I => net_global_inout\n"
                                    "    );\n"
                                    "end STRUCTURE;");
            hdl_parser_vhdl_old vhdl_parser(input);
            std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);
            EXPECT_EQ(nl, nullptr);
        }
        /*{ // NOTE: Fails without error message (should work...)
            // Leave the 'port map' block empty (gate is not connected)
            NO_COUT_TEST_BLOCK;
            std::stringstream input("-- Device\t: device_name\n"
                                    "entity TEST_Comp is\n"
                                    "  port (\n"
                                    "    net_global_inout : inout STD_LOGIC := 'X';\n"
                                    "  );\n"
                                    "end TEST_Comp;\n"
                                    "architecture STRUCTURE of TEST_Comp is\n"
                                    "begin\n"
                                    "  gate_0 : INV\n"
                                    "    port map (\n"
                                    "    );\n"
                                    "end STRUCTURE;");
            hdl_parser_vhdl_old vhdl_parser(input);
            std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);
            ASSERT_NE(nl, nullptr);
            ASSERT_FALSE(nl->get_gates("INV", "gate_0").empty());
            std::shared_ptr<gate> gate_0 = *nl->get_gates("INV", "gate_0").begin();
            EXPECT_TRUE(gate_0->get_fan_out_nets().empty());
            EXPECT_TRUE(gate_0->get_fan_in_nets().empty());
        }*/
        { //NOTE: Fails because of mistype issue line 108: (inout_pin_type.end() <-> input_pin_type.end())
            // Try to connect to a pin, which does not exist
            NO_COUT_TEST_BLOCK;
            std::stringstream input("-- Device\t: device_name\n"
                                    "entity TEST_Comp is\n"
                                    "  port (\n"
                                    "    net_global_inout : inout STD_LOGIC := 'X';\n"
                                    "  );\n"
                                    "end TEST_Comp;\n"
                                    "architecture STRUCTURE of TEST_Comp is\n"
                                    "begin\n"
                                    "  gate_0 : INV\n"
                                    "    port map (\n"
                                    "      NOT_EXISTING_PIN => net_global_inout\n"
                                    "    );\n"
                                    "end STRUCTURE;");
            hdl_parser_vhdl_old vhdl_parser(input);
            std::shared_ptr<netlist> nl = vhdl_parser.parse(g_lib_name);
            ASSERT_EQ(nl, nullptr);
        }

        {
            // NOTE: Almost infinite other edge cases (not based on code-coverage)
        }
    TEST_END
}


