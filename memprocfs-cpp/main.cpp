#include "functions.h"

#include "json.h"

int main()
{
    std::fstream ifs("config.json");
    if (!ifs.is_open())
    {
        printf("[-] failed to open config.json\n");
        return 0;
    }

    nlohmann::json j = nlohmann::json::parse(ifs);

    ifs.close();

    bool use_memory_map = (bool)j["use_memory_map"];
    std::string memory_map_location = (std::string)j["memory_map_location"];

    j.clear();

    std::vector<const char*> args = { "", "-device", "FPGA" };
    if (use_memory_map)
    {
        args.push_back("-memmap");
        args.push_back(memory_map_location.c_str());
    }

    c_device device = c_device(args);
    if (!device.connect())
        return device.error("[-] failed to connect to device\n");
    else
        printf("[+] connected to device, id -> %lli | version -> %lli.%lli\n\n", device.id, device.major_version, device.minor_version);

    c_process process = device.process_from_name("RustClient.exe");
    if (process.failed)
        return device.error("[-] failed to find rust\n");
    else
        printf("[+] found rust\n");

    module_data_t assembly = process.module_from_name("GameAssembly.dll");
    if (assembly.failed)
        return device.error("[-] failed to find assembly\n");
    else
        printf("[+] found assembly, base -> 0x%llx | size -> 0x%llx\n", assembly.base, assembly.size);

    c_memory memory = process.get_memory();

    uint64_t occlusion_culling = memory.read<uint64_t>(assembly.base + 56978928); //this is script.json file from the dumper
    occlusion_culling = memory.read<uint64_t>(occlusion_culling + 0xB8);
    printf("[+] occlusion -> 0x%llx\n", occlusion_culling);

    int debug_show = memory.read<int>(occlusion_culling + 0x94);
    printf("[+] d-show -> %d(this should be 0)\n", debug_show);

    if (debug_show == 0 && occlusion_culling > 0x10000)
        memory.write<int>(occlusion_culling + 0x94, 1);

    //private static OcclusionCulling.DebugFilter _debugShow; // 0x94 (this is in dump.cs if u need update)

    device.disconnect();

    printf("[+] disconnected device\n");

    getchar();

    return 0;
}