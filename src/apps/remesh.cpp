/**
 * @file        test_app.cpp
 * 
 * @brief       Sample application for testing features.
 * 
 * @author      Filippo Maggioli\n
 *              (maggioli@di.uniroma1.it, maggioli.filippo@gmail.com)\n
 *              Sapienza, University of Rome - Department of Computer Science
 * 
 * @date        2023-07-17
 */
#include <Eigen/Dense>
#include <Eigen/Sparse>

#include <rmt/rmt.hpp>

#include <igl/readOBJ.h>
#include <igl/writeOBJ.h>
#include <unsupported/Eigen/SparseExtra>
#include <nlohmann/json.hpp>

#include <iostream>
#include <fstream>
#include <chrono>
#include <filesystem>

void StartTimer();
double StopTimer();

struct rmtArgs
{
    std::string InMesh;
    std::string OutMesh;
    int NumSamples;
    bool Resampling;
    bool Evaluate;
};

rmtArgs ParseArgs(int argc, const char* const argv[]);
void Usage(const std::string& Prog, bool IsError = false);

int main(int argc, const char* const argv[])
{
    Eigen::MatrixXd V;
    Eigen::MatrixXi F;

    auto Args = ParseArgs(argc, argv);
    double TotTime = 0.0;
    double t = 0.0;


    std::cout << "Loading mesh " << Args.InMesh << "... ";
    StartTimer();
    if (!rmt::LoadMesh(Args.InMesh, V, F))
    {
        std::cerr << "Cannot load mesh." << std::endl;
        return -1;
    }
    t = StopTimer();
    std::cout << "Elapsed time is " << t << " s." << std::endl;

    std::cout << "Number of vertices:  " << V.rows() << std::endl;
    std::cout << "Number of triangles: " << F.rows() << std::endl;

    int nVertsOrig = V.rows();
    auto FOrig = F;
    if (Args.Resampling)
    {
        std::cout << "Applying resampling... ";
        StartTimer();
        double MEL = rmt::MaxEdgeLength(V, F, Args.NumSamples);
        rmt::ResampleMesh(V, F, MEL);
        t = StopTimer();
        TotTime += t;
        std::cout << "Elapsed time is " << t << " s." << std::endl;
        std::cout << "Number of vertices (after resampling):  " << V.rows() << std::endl;
        std::cout << "Number of triangles (after resampling): " << F.rows() << std::endl;
    }

    std::cout << "Building graph... ";
    StartTimer();
    rmt::Graph Graph(V, F);
    t = StopTimer();
    TotTime += t;
    std::cout << "Elapsed time is " << t << " s." << std::endl;

    auto CC = Graph.ConnectedComponents();
    int NumCCs = 0;
    for (int i = 0; i < CC.size(); ++i)
        NumCCs = std::max(NumCCs, CC[i]);
    NumCCs += 1;
    std::cout << "Number of connected components: " << NumCCs << std::endl;

    std::cout << "Remeshing to " << Args.NumSamples << " vertices... ";
    StartTimer();
    std::pair<std::vector<int>, std::vector<int>> VFPS;
    VFPS = rmt::VoronoiFPS(Graph, Args.NumSamples);

    Eigen::MatrixXd VV;
    Eigen::MatrixXi FF;
    rmt::MeshFromVoronoi(Graph, VFPS.first, VFPS.second, VV, FF);
    rmt::ReorientFaces(VFPS.first, V, F, VV, FF);
    t = StopTimer();
    TotTime += t;
    std::cout << "Elapsed time is " << t << " s." << std::endl;

    std::cout << "Total remeshing time is " << TotTime << " s." << std::endl;
    
    std::cout << "Exporting to " << Args.OutMesh << "... ";
    StartTimer();
    if (!rmt::ExportMesh(Args.OutMesh, VV, FF))
    {
        std::cerr << "Cannot write mesh." << std::endl;
        return -1;
    }
    t = StopTimer();
    std::cout << "Elapsed time is " << t << " s." << std::endl;

    if (FF.rows() == 0)
    {
        std::cout << "Sampling density is not enough to capture any face. Maybe there are too many connected components?" << std::endl;
        return 0;
    }

    std::cout << "Computing and exporting the weight map... ";
    StartTimer();
    std::string WMap = Args.OutMesh;
    WMap = WMap.substr(0, WMap.rfind('.')) + ".mat";
    auto W = rmt::WeightMap(V, VV, FF, nVertsOrig);
    rmt::ExportWeightmap(WMap, W);
    t = StopTimer();
    std::cout << "Elapsed time is " << t << " s." << std::endl;


    if (Args.Evaluate)
    {
        std::cout << "Evaluating the remeshing... ";
        StartTimer();
        rmt::RescaleInsideUnitBox(V);
        rmt::RescaleInsideUnitBox(VV);
        auto M = rmt::Evaluate(V, FOrig, VV, FF, nVertsOrig);
        t = StopTimer();
        std::cout << "Elapsed time is " << t << " s." << std::endl;

        std::cout << "Hausdorff distance: " << M.Hausdorff << std::endl;
        std::cout << "Chamfer distance:   " << M.Chamfer << std::endl;
        std::cout << "Triangle area:" << std::endl;
        std::cout << "    Min: " << M.MinArea << std::endl;
        std::cout << "    Max: " << M.MaxArea << std::endl;
        std::cout << "    Avg: " << M.AvgArea << std::endl;
        std::cout << "    Std: " << M.StdArea << std::endl;
        std::cout << "Triangle quality:" << std::endl;
        std::cout << "    Min: " << M.MinQuality << std::endl;
        std::cout << "    Max: " << M.MaxQuality << std::endl;
        std::cout << "    Avg: " << M.AvgQuality << std::endl;
        std::cout << "    Std: " << M.StdQuality << std::endl;
    }



    std::cout << "Program terminated successfully." << std::endl;
    return 0;
}







std::chrono::system_clock::time_point Start;
void StartTimer()
{
    Start = std::chrono::system_clock::now();
}

double StopTimer()
{
    std::chrono::system_clock::time_point End;
    End = std::chrono::system_clock::now();
    std::chrono::system_clock::duration ETA;
    ETA = End - Start;
    size_t ms;
    ms = std::chrono::duration_cast<std::chrono::milliseconds>(ETA).count();
    return ms * 1.0e-3;
}

rmtArgs ParseFromFile(const std::string& Filename)
{
    std::ifstream Stream;
    Stream.open(Filename, std::ios::in);
    if (!Stream.is_open())
    {
        std::cerr << "Cannot open file " << Filename << " for reading." << std::endl;
        exit(-1);
    }

    nlohmann::json j;
    try
    {
        Stream >> j;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        exit(-1);
    }
    
    
    Stream.close();

    if (!j.contains("input_mesh"))
    {
        std::cerr << "Configuration file must contain the \'input_mesh\' attribute." << std::endl;
        exit(-1);
    }
    if (!j.contains("num_samples"))
    {
        std::cerr << "Configuration file must contain the \'num_samples\' attribute." << std::endl;
        exit(-1);
    }

    if (!j["input_mesh"].is_string())
    {
        std::cerr << "\'input_mesh\' attribute must be a string." << std::endl;
        exit(-1);
    }
    if (!j["num_samples"].is_number_integer())
    {
        std::cerr << "\'num_samples\' attribute must be an integer numeric value." << std::endl;
        exit(-1);
    }

    rmtArgs Args;
    Args.InMesh = j["input_mesh"];
    Args.NumSamples = j["num_samples"];
    Args.Resampling = false;
    Args.Evaluate = false;
    Args.OutMesh = std::filesystem::path(Args.InMesh).filename().string();
    Args.OutMesh = (std::filesystem::current_path() / std::filesystem::path(Args.OutMesh)).string();

    if (j.contains("resampling"))
    {
        if (!j["resampling"].is_boolean())
        {
            std::cerr << "When provided, \'resampling\' attribute must be boolean." << std::endl;
            exit(-1);
        }
        Args.Resampling = j["resampling"];
    }

    if (j.contains("evaluate"))
    {
        if (!j["evaluate"].is_boolean())
        {
            std::cerr << "When provided, \'evaluate\' attribute must be boolean." << std::endl;
            exit(-1);
        }
        Args.Evaluate = j["evaluate"];
    }

    if (j.contains("out_mesh"))
    {
        if (!j["out_mesh"].is_string())
        {
            std::cerr << "When provided, \'out_mesh\' attribute must be a string." << std::endl;
            exit(-1);
        }
        Args.OutMesh = j["out_mesh"];
    }

    return Args;
}

rmtArgs ParseArgs(int argc, const char* const argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        std::string argvi(argv[i]);
        if (argvi == "-h" || argvi == "--help")
        {
            Usage(argv[0]);
            exit(0);
        }
    }


    rmtArgs Args;
    Args.InMesh = "";
    Args.OutMesh = "";
    Args.NumSamples = -1;
    Args.Resampling = false;
    Args.Evaluate = false;

    for (int i = 1; i < argc; ++i)
    {
        std::string argvi(argv[i]);
        if (argvi == "-f" || argvi == "--file")
        {
            if (i == argc - 1)
            {
                Usage(argv[0], true);
            }
            return ParseFromFile(argv[i + 1]);
        }
        if (argvi == "-o" || argvi == "--output")
        {
            if (i == argc - 1)
            {
                Usage(argv[0], true);
            }
            Args.OutMesh = argv[++i];
            continue;
        }
        if (argvi == "-r" || argvi == "--resample")
        {
            Args.Resampling = true;
            continue;
        }
        if (argvi == "-e" || argvi == "--evaluate")
        {
            Args.Evaluate = true;
            continue;
        }
        if (Args.InMesh.empty())
            Args.InMesh = argvi;
        else
            Args.NumSamples = std::stoi(argvi);
    }

    if (Args.InMesh.empty())
    {
        std::cerr << "No input mesh given." << std::endl;
        Usage(argv[0], true);
    }
    if (Args.NumSamples == -1)
    {
        std::cerr << "No output size given." << std::endl;
        Usage(argv[0], true);
    }
    if (Args.OutMesh.empty())
    {
        Args.OutMesh = std::filesystem::path(Args.InMesh).filename().string();
        Args.OutMesh = (std::filesystem::current_path() / std::filesystem::path(Args.OutMesh)).string();
    }

    return Args;
}


void Usage(const std::string& Prog, bool IsError)
{
    std::ostream* _out = &std::cout;
    if (IsError)
        _out = &std::cerr;
    std::ostream& out = *_out;

    out << std::endl;
    out << Prog << " usage:" << std::endl;
    out << std::endl;
    out << "\t" << Prog << " input_mesh num_samples [-o|--output out_mesh] [-r|--resample] [-e|--evaluate]" << std::endl;
    out << "\t" << Prog << " -f|--file config_file" << std::endl;
    out << "\t" << Prog << " -h|--help" << std::endl;
    out << std::endl;
    out << "Arguments details:" << std::endl;
    out << "\t- input_mesh is the file containing the mesh to process;" << std::endl;
    out << "\t- num_samples is the size of the output mesh;" << std::endl;
    out << "\t- -o|--output sets the output file to out_mesh, by default the base name of input_mesh in the CWD;" << std::endl;
    out << "\t- -r|--resample applies a resampling of the input mesh for a more uniform remeshing;" << std::endl;
    out << "\t- -e|--evaluate evaluates the resampling quality according to various metrics." << std::endl;
    out << "\t- -f|--file sets the arguments using the content of config_file." << std::endl;
    out << "\t- -h|--help prints this message." << std::endl;

    if (IsError)
        exit(-1);
}