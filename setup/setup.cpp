#include "setup.h"

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
        "localhost", bpo::bool_switch(), "All parties are on same machine.")("vec-size,v", bpo::value<size_t>()->default_value(10),
                                                                             "Number of vector elements.")(
        "certificate_path", bpo::value<std::string>()->default_value("certs/cert1.pem"), "Path to full certificate chain file for TLS server connections")(
        "private_key_path", bpo::value<std::string>()->default_value("certs/key1.pem"), "Path to private key for TLS server connections")(
        "trusted_cert_path", bpo::value<std::string>()->default_value("certs/cert_ca.pem"), "Path with trusted certificate for TLS client connections")(
        "port", bpo::value<int>()->default_value(10000), "Base port for networking.")(
        "nodes", bpo::value<size_t>()->default_value(1000), "Number of nodes for benchmarking graph algorithms")("output,o", bpo::value<std::string>(),
                                                                                                                 "File to save benchmarks.")(
        "repeat,r", bpo::value<size_t>()->default_value(1), "Number of times to run benchmarks.")("num_parties,np", bpo::value<size_t>()->default_value(3),
                                                                                                  "Number of parties running the protocol.")(
        "shuffle_num", bpo::value<size_t>()->default_value(1), "Number of chained shuffle gates per shuffle.");

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

        if (!opts["localhost"].as<bool>() && (opts.count("net-config") == 0)) {
            throw std::runtime_error("Expected one of 'localhost' or 'net-config'");
        }
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << std::endl;
        throw std::runtime_error("Error occured");
    }

    return opts;
}

void setup::setupExecution(const bpo::variables_map &opts, size_t &pid, size_t &nP, size_t &repeat, size_t &threads, size_t &shuffle_num, size_t &nodes,
                           std::shared_ptr<io::NetIOMP> &network, uint64_t *seeds_h, uint64_t *seeds_l, bool &save_output, std::string &save_file) {
    save_output = false;
    if (opts.count("output") != 0) {
        save_output = true;
        save_file = opts["output"].as<std::string>();
    }

    pid = opts["pid"].as<size_t>();
    nP = opts["num_parties"].as<size_t>();
    threads = opts["threads"].as<size_t>();
    shuffle_num = opts["shuffle_num"].as<size_t>();
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

    if (opts["localhost"].as<bool>()) {
        network = std::make_shared<io::NetIOMP>(pid, nP, port, nullptr, certificate_path, private_key_path, trusted_cert_path, true);
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

        network = std::make_shared<io::NetIOMP>(pid, nP, port, ip.data(), certificate_path, private_key_path, trusted_cert_path, false);
    }
}

void setup::run_test(const bpo::variables_map &opts,
                     std::function<void(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE)> func) {
    auto n = opts["vec-size"].as<size_t>();

    size_t pid, nP, repeat, threads, shuffle_num, nodes;
    std::shared_ptr<io::NetIOMP> network = nullptr;
    uint64_t seeds_h[9];
    uint64_t seeds_l[9];
    json output_data;
    bool save_output;
    std::string save_file;

    setup::setupExecution(opts, pid, nP, repeat, threads, shuffle_num, nodes, network, seeds_h, seeds_l, save_output, save_file);
    output_data["details"] = {{"pid", pid},         {"num_parties", nP}, {"threads", threads}, {"seeds_h", seeds_h},
                              {"seeds_l", seeds_l}, {"repeat", repeat},  {"vec-size", n}};

    std::cout << "--- Details ---\n";
    for (const auto &[key, value] : output_data["details"].items()) {
        std::cout << key << ": " << value << "\n";
    }
    std::cout << std::endl;

    Party id = (pid == 0) ? P0 : ((pid == 1) ? P1 : D);
    RandomGenerators rngs(seeds_h, seeds_l);
    const size_t BLOCK_SIZE = 100000;

    func(id, rngs, network, n, BLOCK_SIZE);
}

void setup::run_benchmark(const bpo::variables_map &opts,
                          std::function<void(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, size_t repeat,
                                             size_t n_vertices, bool save_output, std::string save_file)>
                              func) {
    auto n = opts["vec-size"].as<size_t>();

    size_t pid, nP, repeat, threads, shuffle_num, nodes;
    std::shared_ptr<io::NetIOMP> network = nullptr;
    uint64_t seeds_h[9];
    uint64_t seeds_l[9];
    json output_data;
    bool save_output;
    std::string save_file;

    setup::setupExecution(opts, pid, nP, repeat, threads, shuffle_num, nodes, network, seeds_h, seeds_l, save_output, save_file);
    output_data["details"] = {{"pid", pid},         {"num_parties", nP}, {"threads", threads}, {"seeds_h", seeds_h},
                              {"seeds_l", seeds_l}, {"repeat", repeat},  {"vec-size", n}};

    std::cout << "--- Details ---\n";
    for (const auto &[key, value] : output_data["details"].items()) {
        std::cout << key << ": " << value << "\n";
    }
    std::cout << std::endl;

    Party id = (pid == 0) ? P0 : ((pid == 1) ? P1 : D);
    RandomGenerators rngs(seeds_h, seeds_l);
    const size_t BLOCK_SIZE = 100000;

    func(id, rngs, network, n, BLOCK_SIZE, repeat, nodes, save_output, save_file);
}
