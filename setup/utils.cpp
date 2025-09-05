#include "utils.h"

void setup::print_vec(std::vector<Ring> &vec) {
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i != vec.size() - 1) {
            std::cout << vec[i] << ", ";
        } else {
            std::cout << vec[i] << std::endl;
        }
    }
}

bpo::options_description setup::programOptions() {
    bpo::options_description desc(
        "Following options are supported by config file too. Regarding seeds, "
        "all, 01, 02, and 12 need to be equal among the parties using them. "
        "Other parties, e.g., party 2 for seed 01 can take any value and in "
        "fact must not know the value that 0 and 1 use");

    desc.add_options()("pid,p", bpo::value<size_t>()->default_value(0), "Party ID.")(
        "threads,t", bpo::value<size_t>()->default_value(6), "Number of threads (recommended 6).")("seed_self_h", bpo::value<uint64_t>()->default_value(100),
                                                                                                   "Value of the private random seed, high bits.")(
        "seed_self_l", bpo::value<uint64_t>()->default_value(0), "Value of the random seed, low bits (will add pid if 0/default).")(
        "seed_all_h", bpo::value<uint64_t>()->default_value(4), "Value of the global random seed, high bits.")(
        "seed_all_l", bpo::value<uint64_t>()->default_value(8), "Value of the global random seed, low bits.")(
        "seed_01_h", bpo::value<uint64_t>()->default_value(15), "Value of the 01-shared random seed, high bits.")(
        "seed_01_l", bpo::value<uint64_t>()->default_value(16), "Value of the 01-shared random seed, low bits.")(
        "seed_02_h", bpo::value<uint64_t>()->default_value(23), "Value of the 02-shared random seed, high bits.")(
        "seed_02_l", bpo::value<uint64_t>()->default_value(42), "Value of the 02-shared random seed, low bits.")(
        "seed_12_h", bpo::value<uint64_t>()->default_value(108), "Value of the 12-shared random seed, high bits.")(
        "seed_12_l", bpo::value<uint64_t>()->default_value(1337), "Value of the 12-shared random seed, low bits.")(
        "seed_D0_unshuffle_h", bpo::value<uint64_t>()->default_value(21108), "Value of the D0-shared random seed for unshuffle, high bits.")(
        "seed_D0_unshuffle_l", bpo::value<uint64_t>()->default_value(211337), "Value of the D0-shared random seed for unshuffle, low bits.")(
        "seed_D1_unshuffle_h", bpo::value<uint64_t>()->default_value(1108), "Value of the D1-shared random seed for unshuffle, high bits.")(
        "seed_D1_unshuffle_l", bpo::value<uint64_t>()->default_value(11337), "Value of the D1-shared random seed for unshuffle, low bits.")(
        "seed_D0_comp_h", bpo::value<uint64_t>()->default_value(1111108), "Value of the D0-shared random seed for compaction, high bits.")(
        "seed_D0_comp_l", bpo::value<uint64_t>()->default_value(11111337), "Value of the D0-shared random seed for compaction, low bits.")(
        "seed_D1_comp_h", bpo::value<uint64_t>()->default_value(111108), "Value of the D1-shared random seed for compaction, high bits.")(
        "seed_D1_comp_l", bpo::value<uint64_t>()->default_value(1111337), "Value of the D1-shared random seed for compaction, low bits.")(
        "net-config", bpo::value<std::string>(), "Path to JSON file containing network details of all parties.")(
        "localhost", bpo::bool_switch(), "All parties are on same machine.")("size,s", bpo::value<size_t>()->default_value(10), "Number of graph entries.")(
        "certificate_path", bpo::value<std::string>()->default_value("certs/cert1.pem"), "Path to full certificate chain file for TLS server connections")(
        "private_key_path", bpo::value<std::string>()->default_value("certs/key1.pem"), "Path to private key for TLS server connections")(
        "trusted_cert_path", bpo::value<std::string>()->default_value("certs/cert_ca.pem"), "Path with trusted certificate for TLS client connections")(
        "port", bpo::value<int>()->default_value(10000), "Base port for networking.")(
        "nodes", bpo::value<size_t>()->default_value(5), "Number of nodes for benchmarking graph algorithms")("output,o", bpo::value<std::string>(),
                                                                                                              "File to save benchmarks.")(
        "repeat,r", bpo::value<size_t>()->default_value(1), "Number of times to run benchmarks.")("num_parties,np", bpo::value<size_t>()->default_value(3),
                                                                                                  "Number of parties running the protocol.")(
        "BLOCK_SIZE", bpo::value<size_t>()->default_value(1000000), "BLOCK_SIZE for sending messages over the network.")(
        "ssd", bpo::bool_switch(), "Preprocessing values are stored on disk before they are sent.")(
        "input,i", bpo::value<std::string>(), "File specifying the graph.")("depth,d", bpo::value<size_t>()->default_value(0), "search depth parameter D")(
        "clients", bpo::value<size_t>()->default_value(0), "Number of clients participating in input-sharing.")(
        "passwords", bpo::value<std::string>()->default_value(""), "Path to .txt file containing passwords used for authenticating clients.")(
        "input-port", bpo::value<int>()->default_value(4242), "Port used for receiving input shares from clients.");

    return desc;
}

bpo::options_description setup::clientProgramOptions() {
    bpo::options_description desc(
        "Following options are supported by config file too. Regarding seeds, "
        "all, 01, 02, and 12 need to be equal among the parties using them. "
        "Other parties, e.g., party 2 for seed 01 can take any value and in "
        "fact must not know the value that 0 and 1 use");

    desc.add_options()("cid", bpo::value<int>()->default_value(0), "Client ID.")("start", bpo::value<size_t>()->default_value(0), "Start index of subgraph")(
        "input,i", bpo::value<std::string>(), "Input file from which subgraph is read.")("ip_0", bpo::value<std::string>(), "IP Address of P0.")(
        "ip_1", bpo::value<std::string>(), "IP Address of P1.")("port_0", bpo::value<int>()->default_value(4242), "Port of P0.")(
        "password", bpo::value<std::string>()->default_value(""), "Password used by client for authentication.")(
        "port_1", bpo::value<int>()->default_value(4243), "Port of P1.")("n_bits", bpo::value<size_t>()->default_value(32),
                                                                         "Number of bits used for representing ring elements.");

    return desc;
}

bpo::variables_map setup::parseOptions(bpo::options_description &cmdline, bpo::options_description &prog_opts, int argc, char *argv[]) {
    bpo::variables_map opts;
    bpo::store(bpo::command_line_parser(argc, argv).options(cmdline).run(), opts);

    if (opts.count("help") != 0) {
        std::cout << cmdline << std::endl;
        return 0;
    }

    if (opts.count("config") > 0) {
        std::string cpath(opts["config"].as<std::string>());
        std::ifstream fin(cpath.c_str());

        if (fin.fail()) {
            std::cerr << "Could not open configuration file at " << cpath << "\n";
            throw std::runtime_error("Conf file missing");
        }

        bpo::store(bpo::parse_config_file(fin, prog_opts), opts);
    }

    try {
        bpo::notify(opts);

        // if (!opts["localhost"].as<bool>() && (opts.count("net-config") == 0)) {
        // throw std::runtime_error("Expected one of 'localhost' or 'net-config'");
        //}

    } catch (const std::exception &ex) {
        std::cerr << ex.what() << std::endl;
        throw std::runtime_error("Error occured");
    }

    return opts;
}

void setup::setupClient(const bpo::variables_map &opts, int &id, size_t &start_idx, std::string &ip_0, std::string &ip_1, int &port_0, int &port_1,
                        std::string &input_file, size_t &n_bits, std::string &password) {
    id = opts["cid"].as<int>();
    start_idx = opts["start"].as<size_t>();
    ip_0 = opts["ip_0"].as<std::string>();
    ip_1 = opts["ip_1"].as<std::string>();
    port_0 = opts["port_0"].as<int>();
    port_1 = opts["port_1"].as<int>();
    input_file = opts["input"].as<std::string>();
    n_bits = opts["n_bits"].as<size_t>();
    password = opts["password"].as<std::string>();
}

void setup::setupExecution(const bpo::variables_map &opts, size_t &pid, size_t &nP, size_t &nC, size_t &repeat, size_t &threads, size_t &nodes,
                           io::NetworkConfig &net_conf, uint64_t *seeds_h, uint64_t *seeds_l, bool &save_output, std::string &save_file, bool &save_to_disk,
                           std::string &input_file, std::string &passwords_file, int &input_port) {
    save_output = false;
    if (opts.count("output") != 0) {
        save_output = true;
        save_file = opts["output"].as<std::string>();
    }

    if (opts.count("input") != 0) {
        input_file = opts["input"].as<std::string>();
    }

    if (opts.count("passwords") != 0) {
        passwords_file = opts["passwords"].as<std::string>();
    }

    pid = opts["pid"].as<size_t>();
    nP = opts["num_parties"].as<size_t>();
    nC = opts["clients"].as<size_t>();
    threads = opts["threads"].as<size_t>();
    nodes = opts["nodes"].as<size_t>();
    seeds_h[0] = opts["seed_self_h"].as<uint64_t>();
    seeds_h[1] = opts["seed_all_h"].as<uint64_t>();
    seeds_h[2] = opts["seed_01_h"].as<uint64_t>();
    seeds_h[3] = opts["seed_02_h"].as<uint64_t>();
    seeds_h[4] = opts["seed_12_h"].as<uint64_t>();
    seeds_h[5] = opts["seed_D0_unshuffle_h"].as<uint64_t>();
    seeds_h[6] = opts["seed_D1_unshuffle_h"].as<uint64_t>();
    seeds_h[7] = opts["seed_D0_comp_h"].as<uint64_t>();
    seeds_h[8] = opts["seed_D1_comp_h"].as<uint64_t>();
    seeds_l[0] = opts["seed_self_l"].as<uint64_t>();
    if (seeds_l[0] == 0) {
        // Default value, but as this is own seed, make unique per party
        seeds_l[0] = pid;
    }
    seeds_l[1] = opts["seed_all_l"].as<uint64_t>();
    seeds_l[2] = opts["seed_01_l"].as<uint64_t>();
    seeds_l[3] = opts["seed_02_l"].as<uint64_t>();
    seeds_l[4] = opts["seed_12_l"].as<uint64_t>();
    seeds_l[5] = opts["seed_D0_unshuffle_l"].as<uint64_t>();
    seeds_l[6] = opts["seed_D1_unshuffle_l"].as<uint64_t>();
    seeds_l[7] = opts["seed_D0_comp_l"].as<uint64_t>();
    seeds_l[8] = opts["seed_D1_comp_l"].as<uint64_t>();

    repeat = opts["repeat"].as<size_t>();
    auto port = opts["port"].as<int>();

    auto certificate_path = opts["certificate_path"].as<std::string>();
    auto private_key_path = opts["private_key_path"].as<std::string>();
    auto trusted_cert_path = opts["trusted_cert_path"].as<std::string>();
    size_t BLOCK_SIZE = opts["BLOCK_SIZE"].as<size_t>();

    net_conf.id = (Party)pid;
    net_conf.n_parties = (int)nP;
    net_conf.port = port;
    net_conf.certificate_path = certificate_path;
    net_conf.private_key_path = private_key_path;
    net_conf.trusted_cert_path = trusted_cert_path;
    net_conf.BLOCK_SIZE = BLOCK_SIZE;

    if (opts["localhost"].as<bool>()) {
        net_conf.IP = (char **)nullptr;
        net_conf.localhost = true;

    } else {
        std::ifstream fnet(opts["net-config"].as<std::string>());
        if (!fnet.good()) {
            fnet.close();
            throw std::runtime_error("Could not open network config file");
        }
        json netdata;
        fnet >> netdata;
        fnet.close();

        std::vector<std::string> ipaddress(3);
        std::array<char *, 5> ip{};
        for (size_t i = 0; i < 3; ++i) {
            ipaddress[i] = netdata[i].get<std::string>();
            ip[i] = ipaddress[i].data();
        }

        net_conf.IP = ip.data();
        net_conf.localhost = false;
    }

    save_to_disk = opts["ssd"].as<bool>();
    input_port = opts["input-port"].as<int>();
    if (pid == 1 && input_port == 4242) {
        input_port++;
    }
}

void setup::run_test(const bpo::variables_map &opts,
                     std::function<void(Party id, RandomGenerators &rngs, io::NetworkConfig &net_conf, size_t n, std::string input_file, Graph &g)> func) {
    auto n = opts["size"].as<size_t>();

    size_t pid, nP, nC, repeat, threads, shuffle_num, nodes;
    json output_data;
    io::NetworkConfig net_conf;
    uint64_t seeds_h[9];
    uint64_t seeds_l[9];
    bool save_output;
    std::string save_file;
    bool save_to_disk;
    std::string input_file;
    std::string passwords_file;
    int input_port;

    setup::setupExecution(opts, pid, nP, nC, repeat, threads, nodes, net_conf, seeds_h, seeds_l, save_output, save_file, save_to_disk, input_file,
                          passwords_file, input_port);

    size_t n_bits = std::ceil(std::log2(nodes));
    output_data["details"] = {{"pid", pid},
                              {"seeds_h", seeds_h},
                              {"seeds_l", seeds_l},
                              {"repeat", repeat},
                              {"size", n},
                              {"bits", n_bits},
                              {"nodes", nodes},
                              {"edges", (n - nodes)},
                              {"SSD utilization", save_to_disk}};

    std::cout << "--- Details ---\n";
    for (const auto &[key, value] : output_data["details"].items()) {
        std::cout << key << ": " << value << "\n";
    }
    std::cout << std::endl;

    Party id = (pid == 0) ? P0 : ((pid == 1) ? P1 : D);
    RandomGenerators rngs(seeds_h, seeds_l);
    Graph g;

    if (nC > 0) {
        if (id != D) {
            std::cout << "Using pwds from " << passwords_file << std::endl;
            InputServer server(id, passwords_file, std::to_string(input_port), nC, n_bits);  // Server expecting two clients
            std::cout << "Awaiting " << nC << " packets at " << "localhost:" << std::to_string(input_port) << std::endl;
            server.connect_clients();
            g = server.recv_graph();
            g.print();
        }
    } else if (input_file != "" && id != D) {
        try {
            g = Graph::parse(input_file);
        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
            return;
        }
    }

    func(id, rngs, net_conf, n, input_file, g);
}

void setup::run_benchmark(const bpo::variables_map &opts,
                          std::function<void(Party id, RandomGenerators &rngs, io::NetworkConfig &net_conf, size_t n, size_t repeat, size_t n_vertices,
                                             bool save_output, std::string save_file, bool save_to_disk, std::string input_file, Graph &g)>
                              func) {
    auto n = opts["size"].as<size_t>();

    json output_data;
    size_t pid, nP, nC, repeat, threads, nodes;
    io::NetworkConfig net_conf;
    uint64_t seeds_h[9];
    uint64_t seeds_l[9];
    bool save_output;
    std::string save_file;
    bool save_to_disk;
    std::string input_file;
    std::string passwords_file;
    int input_port;

    setup::setupExecution(opts, pid, nP, nC, repeat, threads, nodes, net_conf, seeds_h, seeds_l, save_output, save_file, save_to_disk, input_file,
                          passwords_file, input_port);
    size_t n_bits = std::ceil(std::log2(nodes));
    output_data["details"] = {{"pid", pid},
                              {"seeds_h", seeds_h},
                              {"seeds_l", seeds_l},
                              {"repeat", repeat},
                              {"size", n},
                              {"bits", n_bits},
                              {"nodes", nodes},
                              {"edges", (n - nodes)},
                              {"SSD utilization", save_to_disk}};

    std::cout << "--- Details ---\n";
    for (const auto &[key, value] : output_data["details"].items()) {
        std::cout << key << ": " << value << "\n";
    }
    std::cout << std::endl;

    Party id = (pid == 0) ? P0 : ((pid == 1) ? P1 : D);
    RandomGenerators rngs(seeds_h, seeds_l);

    Graph g;
    if (nC > 0) {
        if (id != D) {
            std::cout << "Using pwds from " << passwords_file << std::endl;
            InputServer server(id, passwords_file, std::to_string(input_port), nC, n_bits);
            std::cout << "Awaiting " << nC << " packets at " << "localhost:" << std::to_string(input_port) << std::endl;
            server.connect_clients();
            g = server.recv_graph();
            g.print();
        }
    } else if (input_file != "" && id != D) {
        try {
            g = Graph::parse(input_file);
        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
            return;
        }
    }

    func(id, rngs, net_conf, n, repeat, nodes, save_output, save_file, save_to_disk, input_file, g);
}
