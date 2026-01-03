//
// Created by phosg on 4/20/2025.
//

#include "Core/Types.h"
#include "Core/Common.h"
#include "Core/Log.h"
#include "Core/Memory.h"

#include "Rendering/Shader.h"

#include "Tools/ShaderCompiler.h"

#include <slang/slang.h>
#include <slang/slang-com-ptr.h>

namespace zp::ShaderCompiler
{
    namespace
    {
        slang::IGlobalSession* g_globalSession;
        const char* kOne = "1";
        const char* kZero = "0";

#pragma region Inline Compute Code
        const char* kComputeSampleSlang = R"SLANG(
// compute-simple.slang

static const uint THREADGROUP_SIZE_X = 8;
static const uint THREADGROUP_SIZE_Y = THREADGROUP_SIZE_X;

struct ImageProcessingOptions
{
    float3 tintColor;
    float blurRadius;

    bool useLookupTable;
    StructuredBuffer<float4> lookupTable;
}

[shader("compute")]
[numthreads(THREADGROUP_SIZE_X, THREADGROUP_SIZE_Y)]
void ComputeMain(
    uint3 threadID : SV_DispatchThreadID,
    uniform Texture2D inputImage,
    uniform RWTexture2D outputImage,
    uniform ImageProcessingOptions options)
{
    /* actual logic would go here */
}
)SLANG";
#pragma endregion

#pragma region Inline Raster Code
        const char* kRasterSampleSlang = R"SLANG(
// shaders.slang

//
// This example builds on the simplistic shaders presented in the
// "Hello, World" example by adding support for (intentionally
// simplistic) surface materil and light shading.
//
// The code here is not meant to exemplify state-of-the-art material
// and lighting techniques, but rather to show how a shader
// library can be developed in a modular fashion without reliance
// on the C preprocessor manual parameter-binding decorations.
//

// We are going to define a simple model for surface material shading.
//
// The first building block in our model will be the representation of
// the geometry attributes of a surface as fed into the material.
//
struct SurfaceGeometry
{
    float3 position;
    float3 normal;

    // TODO: tangent vectors would be the natural next thing to add here,
    // and would be required for anisotropic materials. However, the
    // simplistic model loading code we are currently using doesn't
    // produce tangents...
    //
    //      float3 tangentU;
    //      float3 tangentV;

    // We store a single UV parameterization in these geometry attributes.
    // A more complex renderer might need support for multiple UV sets,
    // and indeed it might choose to use interfaces and generics to capture
    // the different requirements that different materials impose on
    // the available surface attributes. We won't go to that kind of
    // trouble for such a simple example.
    //
    float2 uv;
};
//
// Next, we want to define the fundamental concept of a refletance
// function, so that we can use it as a building block for other
// parts of the system. This is a case where we are trying to
// show how a proper physically-based renderer (PBR) might
// decompose the problem using Slang, even though our simple
// example is *not* physically based.
//
interface IBRDF
{
    // Technically, a BRDF is only a function of the incident
    // (`wi`) and exitant (`wo`) directions, but for simplicity
    // we are passing in the surface normal (`N`) as well.
    //
    float3 evaluate(float3 wo, float3 wi, float3 N);
};
//
// We can now define various implemntations of the `IBRDF` interface
// that represent different reflectance functions we want to support.
// For now we keep things simple by defining about the simplest
// reflectance function we can think of: the Blinn-Phong reflectance
// model:
//
struct BlinnPhong : IBRDF
{
    // Blinn-Phong needs diffuse and specular reflectances, plus
    // a specular exponent value (which relates to "roughness"
    // in more modern physically-based models).
    //
    float3 kd;
    float3 ks;
    float specularity;

    // Here we implement the one requirement of the `IBRDF` interface
    // for our concrete implementation, using a textbook definition
    // of Blinng-Phong shading.
    //
    // Note: our "BRDF" definition here folds the N-dot-L term into
    // the evlauation of the reflectance function in case there are
    // useful algebraic simplifications this enables.
    //
    float3 evaluate(float3 V, float3 L, float3 N)
    {
        float nDotL = saturate(dot(N, L));
        float3 H = normalize(L + V);
        float nDotH = saturate(dot(N, H));

        // TODO: The current model loading has a bug that is leading
        // to the `ks` and `specularity` fields being invalid garbage
        // for our example cube, and the result is a non-finite value
        // coming out of `evaluate()` if we include the specular term.

        // return kd*nDotL + ks*pow(nDotH, specularity);
        return kd*nDotL;
    }
};
//
// It is important to note that a reflectance function is *not*
// a "material." In most cases, a material will have spatially-varying
// properties so that it cannot be summarized as a single `IBRDF`
// instance.
//
// Thus a "material" is a value that can produce a BRDF for any point
// on a surface (e.g., by sampling texture maps, etc.).
//
interface IMaterial
{
    // Different concrete material implementations might yield BRDF
    // values with different types. E.g., one material might yield
    // reflectance functions using `BlinnPhong` while another uses
    // a much more complicated/accurate representation.
    //
    // We encapsulate the choice of BRDF parameters/evaluation in
    // our material interface with an "associated type." In the
    // simplest terms, think of this as an interface requirement
    // that is a type, instead of a method.
    //
    // (If you are C++-minded, you might think of this as akin to
    // how every container provided an `iterator` type, but different
    // containers may have different types of iterators)
    //
    associatedtype BRDF : IBRDF;

    // For our simple example program, it is enough for a material to
    // be able to return a BRDF given a point on the surface.
    //
    // A more complex implementation of material shading might also
    // have the material return updated surface geometry to reflect
    // the result of normal mapping, occlusion mapping, etc. or
    // return an opacity/coverage value for partially transparent
    // surfaces.
    //
    BRDF prepare(SurfaceGeometry geometry);
};

// We will now define a trivial first implementation of the material
// interface, which uses our Blinn-Phong BRDF with uniform values
// for its parameters.
//
// Note that this implemetnation is being provided *after* the
// shader parameter `gMaterial` is declared, so that there is no
// assumption in the shader code that `gMaterial` will be plugged
// in using an instance of `SimpleMaterial`
//
struct SimpleMaterial : IMaterial
{
    // We declare the properties we need as fields of the material type.
    // When `SimpleMaterial` is used for `TMaterial` above, then
    // `gMaterial` will be a `ParameterBlock<SimpleMaterial>`, and these
    // parameters will be allocated to a constant buffer that is part of
    // that parameter block.
    //
    // TODO: A future version of this example will include texture parameters
    // here to show that they are declared just like simple uniforms.
    //
    float3 diffuseColor;
    float3 specularColor;
    float specularity;

    // To satisfy the requirements of the `IMaterial` interface, our
    // material type needs to provide a suitable `BRDF` type. We
    // do this by using a simple `typedef`, although a nested
    // `struct` type can also satisfy an associated type requirement.
    //
    // A future version of the Slang compiler may allow the "right"
    // associated type definition to be inferred from the signature
    // of the `prepare()` method below.
    //
    typedef BlinnPhong BRDF;

    BlinnPhong prepare(SurfaceGeometry geometry)
    {
        BlinnPhong brdf;
        brdf.kd = diffuseColor;
        brdf.ks = specularColor;
        brdf.specularity = specularity;
        return brdf;
    }
};
//
// Note that no other code in this file statically
// references the `SimpleMaterial` type, and instead
// it is up to the application to "plug in" this type,
// or another `IMaterial` implementation for the
// `TMaterial` parameter.
//

// A light, or an entire lighting *environment* is an object
// that can illuminate a surface using some BRDF implemented
// with our abstractions above.
//
interface ILightEnv
{
    // The `illuminate` method is intended to integrate incoming
    // illumination from this light (environment) incident at the
    // surface point given by `g` (which has the reflectance function
    // `brdf`) and reflected into the outgoing direction `wo`.
    //
    float3 illuminate<B:IBRDF>(SurfaceGeometry g, B brdf, float3 wo);
    //
    // Note that the `illuminate()` method is allowed as an interface
    // requirement in Slang even though it is a generic. Contrast that
    // with C++ where a `template` method cannot be `virtual`.
};

// Given the `ILightEnv` interface, we can write up almost textbook
// definition of directional and point lights.

struct DirectionalLight : ILightEnv
{
    float3 direction;
    float3 intensity;

    float3 illuminate<B:IBRDF>(SurfaceGeometry g, B brdf, float3 wo)
    {
        return intensity * brdf.evaluate(wo, direction, g.normal);
    }
};
struct PointLight : ILightEnv
{
    float3 position;
    float3 intensity;

    float3 illuminate<B:IBRDF>(SurfaceGeometry g, B brdf, float3 wo)
    {
        float3 delta = position - g.position;
        float d = length(delta);
        float3 direction = normalize(delta);
        float3 illuminance = intensity / (d*d);
        return illuminance * brdf.evaluate(wo, direction, g.normal);
    }
};

// In most cases, a shader entry point will only be specialized for a single
// material, but interesting rendering almost always needs multiple lights.
// For that reason we will next define types to represent *composite* lighting
// environment with multiple lights.
//
// A naive approach might be to have a single undifferntiated list of lights
// where any type of light may appear at any index, but this would lose all
// of the benefits of static specialization: we would have to perform dynamic
// branching to determine what kind of light is stored at each index.
//
// Instead, we will start with a type for *homogeneous* arrays of lights:
//
struct LightArray<L : ILightEnv, let N : int> : ILightEnv
{
    // The `LightArray` type has two generic parameters:
    //
    // - `L` is a type parameter, representing the type of lights that will be in our array
    // - `N` is a generic *value* parameter, representing the maximum number of lights allowed
    //
    // Slang's support for generic value parameters is currently experimental,
    // and the syntax might change.

    int count;
    L lights[N];

    float3 illuminate<B:IBRDF>(SurfaceGeometry g, B brdf, float3 wo)
    {
        // Our light array integrates illumination by naively summing
        // contributions from all the lights in the array (up to `count`).
        //
        // A more advanced renderer might try apply sampling techniques
        // to pick a subset of lights to sample.
        //
        float3 sum = 0;
        for( int ii = 0; ii < count; ++ii )
        {
            sum += lights[ii].illuminate(g, brdf, wo);
        }
        return sum;
    }
};

// `LightArray` can handle multiple lights as long as they have the
// same type, but we need a way to have a scene with multiple lights
// of different types *without* losing static specialization.
//
// The `LightPair<T,U>` type supports this in about the simplest way
// possible, by aggregating a light (environment) of type `T` and
// one of type `U`. Those light environments might themselves be
// `LightArray`s or `LightPair`s, so that arbitrarily complex
// environments can be created from just these two composite types.
//
// This is probably a good place to insert a reminder the Slang's
// generics are *not* C++ templates, so that the error messages
// produced when working with these types are in general reasonable,
// and this is *not* any form of "template metaprogramming."
//
// That said, we expect that future versions of Slang will make
// defining composite types light this a bit less cumbersome.
//
struct LightPair<T : ILightEnv, U : ILightEnv> : ILightEnv
{
    T first;
    U second;

    float3 illuminate<B:IBRDF>(SurfaceGeometry g, B brdf, float3 wo)
    {
        return first.illuminate(g, brdf, wo)
            + second.illuminate(g, brdf, wo);
    }
};

// As a final (degenerate) case, we will define a light
// environment with *no* lights, which contributes no illumination.
//
struct EmptyLightEnv : ILightEnv
{
    float3 illuminate<B:IBRDF>(SurfaceGeometry g, B brdf, float3 wo)
    {
        return 0;
    }
};

// The code above constitutes the "shader library" for our
// application, while the code below this point is the
// implementation of a simple forward rendering pass
// using that library.
//
// While the shader library has used many of Slang's advanced
// mechanisms, the vertex and fragment shaders will be
// much more modest, and hopefully easier to follow.


// We will start with a `struct` for per-view parameters that
// will be allocated into a `ParameterBlock`.
//
// As written, this isn't very different from using an HLSL
// `cbuffer` declaration, but importantly this code will
// continue to work if we add one or more resources (e.g.,
// an enironment map texture) to the `PerView` type.
//
struct PerView
{
    float4x4    viewProjection;
    float3      eyePosition;
};
ParameterBlock<PerView>     gViewParams;

// Declaring a block for per-model parameter data is
// similarly simple.
//
struct PerModel
{
    float4x4    modelTransform;
    float4x4    inverseTransposeModelTransform;
};
ParameterBlock<PerModel>    gModelParams;

// We want our shader to work with any kind of lighting environment
// - that is, and type that implements `ILightEnv`.

uniform ILightEnv gLightEnv;

// Our handling of the material parameter for our shader
// is quite similar to the case for the lighting environment:
//
uniform IMaterial gMaterial;

// Our vertex shader entry point is only marginally more
// complicated than the Hello World example. We will
// start by declaring the various "connector" `struct`s.
//
struct AssembledVertex
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : UV;
};
struct CoarseVertex
{
    float3 worldPosition;
    float3 worldNormal;
    float2 uv;
};
struct VertexStageOutput
{
    CoarseVertex    coarseVertex    : CoarseVertex;
    float4          sv_position     : SV_Position;
};

// Perhaps most interesting new feature of the entry
// point decalrations is that we use a `[shader(...)]`
// attribute (as introduced in HLSL Shader Model 6.x)
// in order to tag our entry points.
//
// This attribute informs the Slang compiler which
// functions are intended to be compiled as shader
// entry points (and what stage they target), so that
// the programmer no longer needs to specify the
// entry point name/stage through the API (or on
// the command line when using `slangc`).
//
// While HLSL added this feature only in newer versions,
// the Slang compiler supports this attribute across
// *all* targets, so that it is okay to use whether you
// want DXBC, DXIL, or SPIR-V output.
//
[shader("vertex")]
VertexStageOutput vertexMain(
    AssembledVertex assembledVertex)
{
    return (VertexStageOutput)0;

    VertexStageOutput output;

    float3 position = assembledVertex.position;
    float3 normal   = assembledVertex.normal;
    float2 uv       = assembledVertex.uv;

    float3 worldPosition = mul(gModelParams.modelTransform, float4(position, 1.0)).xyz;
    float3 worldNormal = mul(gModelParams.inverseTransposeModelTransform, float4(normal, 0.0)).xyz;

    output.coarseVertex.worldPosition = worldPosition;
    output.coarseVertex.worldNormal   = worldNormal;
    output.coarseVertex.uv            = uv;

    output.sv_position = mul(gViewParams.viewProjection, float4(worldPosition, 1.0));

    return output;
}

// Our fragment shader is almost trivial, with the most interesting
// thing being how it uses the `TMaterial` type parameter (through the
// value stored in the `gMaterial` parameter block) to dispatch to
// the correct implementation of the `getDiffuseColor()` method
// in the `IMaterial` interface.
//
// The `gMaterial` parameter block declaration thus serves not only
// to group certain shader parameters for efficient CPU-to-GPU
// communication, but also to select the code that will execute
// in specialized versions of the `fragmentMain` entry point.
//
[shader("fragment")]
float4 fragmentMain(
    CoarseVertex coarseVertex : CoarseVertex) : SV_Target
{
    return 0;
}

float4 fragmentMainOld(
    CoarseVertex coarseVertex : CoarseVertex) : SV_Target
{
    // We start by using our interpolated vertex attributes
    // to construct the local surface geometry that we will
    // use for material evaluation.
    //
    SurfaceGeometry g;
    g.position = coarseVertex.worldPosition;
    g.normal = normalize(coarseVertex.worldNormal);
    g.uv = coarseVertex.uv;

    float3 V = normalize(gViewParams.eyePosition - g.position);

    // Next we prepare the material, which involves running
    // any "pattern generation" logic of the material (e.g.,
    // sampling and blending texture layers), to produce
    // a BRDF suitable for evaluating under illumination
    // from different light sources.
    //
    // Note that the return type here is `gMaterial.BRDF`,
    // which is the `BRDF` type *associated* with the (unknown)
    // `TMaterial` type. When `TMaterial` gets substituted for
    // a concrete type later (e.g., `SimpleMaterial`) this
    // will resolve to a concrete type too (e.g., `SimpleMaterial.BRDF`
    // which is an alias for `BlinnPhong`).
    //
    let brdf = gMaterial.prepare(g);

    // Now that we've done the first step of material evaluation
    // and sampled texture maps, etc., it is time to start
    // integrating incident light at our surface point.
    //
    // Because we've wrapped up the lighting environment as
    // a single (composite) object, this is as simple as calling
    // its `illuminate()` method. Our particular fragment shader
    // is thus abstracted from how the renderer chooses to structure
    // this integration step, somewhat similar to how an
    // `illuminance` loop in RenderMan Shading Language works.
    //

    float3 color = saturate(gLightEnv.illuminate(g, brdf, V) + float3(0.3));
    return float4(color, 1);
}
)SLANG";
#pragma endregion

        void LogDiagError( const Slang::ComPtr<slang::IBlob>& diag )
        {
            if( diag != nullptr )
            {
                const zp_size_t size = diag->getBufferSize();
                const char* str = static_cast<const char*>(diag->getBufferPointer());
                if( str != nullptr && size > 0 )
                {
                    Log::error() << size << ": " << str << Log::endl;
                }
            }
        }

        void DumpToFile( const char* filename, const Slang::ComPtr<slang::IBlob>& data )
        {
            if( data != nullptr )
            {
                Log::info() << "Dump " << filename << " sz" << data->getBufferSize() << Log::endl;
                auto fh = Platform::OpenFileHandle( filename, ZP_OPEN_FILE_MODE_WRITE, ZP_CREATE_FILE_MODE_CREATE );
                Platform::WriteFile( fh, data->getBufferPointer(), data->getBufferSize() );
                Platform::CloseFileHandle( fh );
            }
        }

        void LogPerformance( const Slang::ComPtr<slang::ICompileRequest>& compileRequest, Slang::ComPtr<ISlangProfiler>& slangProfiler )
        {
            compileRequest->getCompileTimeProfile( slangProfiler.writeRef(), true );

            for( zp_size_t i = 0, imax = slangProfiler->getEntryCount(); i < imax; ++i )
            {
                const char* entryName = slangProfiler->getEntryName( i );
                const zp_int64_t ms = slangProfiler->getEntryTimeMS( i );

                Log::info() << entryName << ": " << ms << Log::endl;
            }
        }
    }

    void Initialize()
    {
#if 1
        ZP_ASSERT( SLANG_SUCCEEDED( slang::createGlobalSession( &g_globalSession ) ) );
#else
        ZP_ASSERT( SLANG_SUCCEEDED( slang_createGlobalSessionWithoutStdLib( SLANG_API_VERSION, &g_globalSession ) ) );
#endif
        Log::info() << "Initialized Slang " << g_globalSession->getBuildTagString() << Log::endl;
    }

    void Shutdown()
    {
        ZP_ASSERT( g_globalSession );
        g_globalSession->release();
        g_globalSession = nullptr;

        slang::shutdown();
    }

    //
    // queue a task that has a single shader file with session per file
    // task spawns multiple requests for each stage x variant x specialization
    // convert reflection to something
    // store program binary and maybe debug info if needed in single file
    // each request writes to file cache with unique hash as name
    // output a unique hash -> result (ok or failed)
    //

    void ConvertReflectionToShader( slang::ProgramLayout* reflection )
    {
        for( SlangUInt p = 0, pmax = reflection->getParameterCount(); p < pmax; ++p )
        {
            slang::VariableLayoutReflection* parameter = reflection->getParameterByIndex( p );
            Log::info() << "Param " <<
                parameter->getName() << ": " <<
                parameter->getBindingIndex() << " @" <<
                parameter->getBindingSpace() << ": [" <<
                parameter->getType()->getName() << "]" <<
                Log::endl;
        }

        if( reflection->getGlobalConstantBufferSize() > 0 )
        {
            Log::info() << "CB " <<
                reflection->getGlobalConstantBufferSize() << "sz @" <<
                reflection->getGlobalConstantBufferBinding() <<
                Log::endl;
        }

        slang::VariableLayoutReflection* globalParams = reflection->getGlobalParamsVarLayout();
        if( globalParams != nullptr )
        {
            Log::info() << "Global Param " <<
                globalParams->getName() << ": " <<
                globalParams->getBindingIndex() << " @" <<
                globalParams->getBindingSpace() << ": [" <<
                globalParams->getType()->getName() << "]" <<
                Log::endl;
        }

        for( SlangUInt e = 0, emax = reflection->getEntryPointCount(); e < emax; ++e )
        {
            slang::EntryPointReflection* entryPoint = reflection->getEntryPointByIndex( e );
            const SlangStage stage = entryPoint->getStage();

            Log::info() << "Entry " << entryPoint->getName() << " " << stage << Log::endl;

            for( SlangUInt p = 0, pmax = entryPoint->getParameterCount(); p < pmax; ++p )
            {
                slang::VariableLayoutReflection* parameter = entryPoint->getParameterByIndex( p );
                slang::TypeReflection* type = parameter->getType();
                Log::info() << "Entry param " <<
                    parameter->getName() << " tname " <<
                    type->getName() << " sn " <<
                    parameter->getSemanticName() << " si " <<
                    parameter->getSemanticIndex() << " kind " <<
                    (int)type->getKind() << " fc " <<
                    type->getFieldCount() <<
                    Log::endl;
            }

            slang::VariableLayoutReflection* result = entryPoint->getResultVarLayout();
            slang::TypeReflection* rtype = result->getType();
            Log::info() << "Result param " <<
                result->getName() << " tname " <<
                rtype->getName() << " sn " <<
                result->getSemanticName() << " si " <<
                result->getSemanticIndex() << " kind " <<
                (int)rtype->getKind() << " fc " <<
                rtype->getFieldCount() <<
                Log::endl;

            switch( stage )
            {
                case SLANG_STAGE_COMPUTE:
                {
                    FixedArray<SlangUInt, 3> groupSizes;
                    entryPoint->getComputeThreadGroupSize( groupSizes.length(), groupSizes.data() );

                    SlangUInt waveSize;
                    entryPoint->getComputeWaveSize( &waveSize );
                    Log::info() << "Compute: GroupSize x" << groupSizes[ 0 ] << " y" << groupSizes[ 1 ] << " z" << groupSizes[ 2 ] << " WaveSize " << waveSize << Log::endl;
                }
                break;

                case SLANG_STAGE_FRAGMENT:
                {
                    const bool anySampleRateInput = entryPoint->usesAnySampleRateInput();
                    Log::info() << "Fragment: any sample rate" << anySampleRateInput << Log::endl;
                }
                break;

                default:
                    break;
            }
        }


    }

    void QueueShaderCompilerTaskRequest()
    {
        SlangResult r;

        SlangCompileFlags compileFlags = 0;
        SlangDebugInfoLevel debugInfoLevel = SLANG_DEBUG_INFO_LEVEL_NONE;
        SlangDebugInfoFormat debugInfoFormat = SLANG_DEBUG_INFO_FORMAT_DEFAULT;
        SlangOptimizationLevel optimizationLevel = SLANG_OPTIMIZATION_LEVEL_DEFAULT;
        int buildType = 0;

        Slang::ComPtr<slang::IBlob> diag;

        switch( buildType )
        {
            case 0: // debug
                compileFlags |= SLANG_COMPILE_FLAG_NO_MANGLING;
                debugInfoLevel = SLANG_DEBUG_INFO_LEVEL_MAXIMAL;
                debugInfoFormat = SLANG_DEBUG_INFO_FORMAT_PDB;
                optimizationLevel = SLANG_OPTIMIZATION_LEVEL_NONE;
                break;

            case 1: // release
                debugInfoLevel = SLANG_DEBUG_INFO_LEVEL_MINIMAL;
                optimizationLevel = SLANG_OPTIMIZATION_LEVEL_HIGH;
                break;

            case 2: // distro
                compileFlags |= SLANG_COMPILE_FLAG_OBFUSCATE;
                debugInfoLevel = SLANG_DEBUG_INFO_LEVEL_NONE;
                optimizationLevel = SLANG_OPTIMIZATION_LEVEL_MAXIMAL;
                break;

            default:
                ZP_INVALID_CODE_PATH();
                break;
        }

        FixedArray kCompilerOptions {
            slang::CompilerOptionEntry {
                .name = slang::CompilerOptionName::DebugInformation,
                .value { .kind = slang::CompilerOptionValueKind::Int, .intValue0 = (int)debugInfoLevel },
            },
            slang::CompilerOptionEntry {
                .name = slang::CompilerOptionName::DebugInformationFormat,
                .value { .kind = slang::CompilerOptionValueKind::Int, .intValue0 = (int)debugInfoFormat },
            },
            slang::CompilerOptionEntry {
                .name = slang::CompilerOptionName::Optimization,
                .value { .kind = slang::CompilerOptionValueKind::Int, .intValue0 = (int)optimizationLevel },
            },
        };

        const SlangCompileTarget compileTarget = SLANG_SPIRV;
        const SlangProfileID profileId = g_globalSession->findProfile( "spirv_1_5" );
        const SlangTargetFlags targetFlags = SLANG_TARGET_FLAG_GENERATE_WHOLE_PROGRAM;

        const FixedArray kTargets {
            slang::TargetDesc {
                .format = compileTarget,
                .profile = profileId,
                .flags = targetFlags,
                .floatingPointMode = SLANG_FLOATING_POINT_MODE_FAST,
                .compilerOptionEntries = kCompilerOptions.data(),
                .compilerOptionEntryCount = kCompilerOptions.length(),
            }
        };
        const FixedArray kIncludePaths {
            ".",
            "..",
        };
        const FixedArray kMacros {
            slang::PreprocessorMacroDesc {
                .name = "ZP_GPU_SHADER",
                .value = kOne
            },
        };

        const slang::SessionDesc sessionDesc {
            .targets = kTargets.data(),
            .targetCount = kTargets.length(),
            .searchPaths = kIncludePaths.data(),
            .searchPathCount = kIncludePaths.length(),
            //.preprocessorMacros = kMacros.data(),
            //.preprocessorMacroCount = kMacros.length(),
        };

        Slang::ComPtr<slang::ISession> session;
        r = g_globalSession->createSession( sessionDesc, session.writeRef() );
        ZP_ASSERT( SLANG_SUCCEEDED( r ) );

        Slang::ComPtr<slang::ICompileRequest> compileRequest;
        r = session->createCompileRequest( compileRequest.writeRef() );
        ZP_ASSERT( SLANG_SUCCEEDED( r ) );

        compileRequest->setCodeGenTarget( SLANG_SPIRV );
        compileRequest->setCompileFlags( compileFlags );
        compileRequest->setDebugInfoLevel( debugInfoLevel );
        compileRequest->setDebugInfoFormat( debugInfoFormat );

        const zp_int32_t tu = compileRequest->addTranslationUnit( SLANG_SOURCE_LANGUAGE_SLANG, "main-translation-unit" );
        compileRequest->addTranslationUnitSourceString( tu, "simple.slang", kRasterSampleSlang );
        const int vep = compileRequest->addEntryPoint( tu, "vertexMain", SLANG_STAGE_VERTEX );
        const int fep = compileRequest->addEntryPoint( tu, "fragmentMain", SLANG_STAGE_FRAGMENT );

        r = compileRequest->getDiagnosticOutputBlob( diag.writeRef() );
        ZP_ASSERT( SLANG_SUCCEEDED( r ) );
        LogDiagError( diag );


        r = compileRequest->compile();
        ZP_ASSERT( SLANG_SUCCEEDED( r ) );



        MutableFixedString64 filename;
        Slang::ComPtr<slang::IBlob> entryPointCode;

        Slang::ComPtr<slang::IComponentType> linkedProgram;
        r = compileRequest->getProgramWithEntryPoints( linkedProgram.writeRef() );
        ZP_ASSERT( SLANG_SUCCEEDED( r ) );

        r = linkedProgram->getEntryPointCode( vep, 0, entryPointCode.writeRef() );
        ZP_ASSERT( SLANG_SUCCEEDED( r ) );

        filename.clear();
        filename.appendFormat( "%d", vep );
        filename.append( "-simple.txt" );
        DumpToFile( filename.c_str(), entryPointCode );

        r = linkedProgram->getEntryPointCode( fep, 0, entryPointCode.writeRef() );
        ZP_ASSERT( SLANG_SUCCEEDED( r ) );

        filename.clear();
        filename.appendFormat( "%d", fep );
        filename.append( "-simple.txt" );
        DumpToFile( filename.c_str(), entryPointCode );
    }

    void QueueShaderCompilerTask(int)
    {
        SlangResult r;

        SlangCompileFlags compileFlags = 0;
        SlangDebugInfoLevel debugInfoLevel = SLANG_DEBUG_INFO_LEVEL_NONE;
        SlangDebugInfoFormat debugInfoFormat = SLANG_DEBUG_INFO_FORMAT_DEFAULT;
        SlangOptimizationLevel optimizationLevel = SLANG_OPTIMIZATION_LEVEL_DEFAULT;
        int buildType = 0;

        switch( buildType )
        {
            case 0: // debug
                compileFlags |= SLANG_COMPILE_FLAG_NO_MANGLING;
                debugInfoLevel = SLANG_DEBUG_INFO_LEVEL_MAXIMAL;
                debugInfoFormat = SLANG_DEBUG_INFO_FORMAT_PDB;
                optimizationLevel = SLANG_OPTIMIZATION_LEVEL_NONE;
                break;

            case 1: // release
                debugInfoLevel = SLANG_DEBUG_INFO_LEVEL_MINIMAL;
                optimizationLevel = SLANG_OPTIMIZATION_LEVEL_HIGH;
                break;

            case 2: // distro
                compileFlags |= SLANG_COMPILE_FLAG_OBFUSCATE;
                debugInfoLevel = SLANG_DEBUG_INFO_LEVEL_NONE;
                optimizationLevel = SLANG_OPTIMIZATION_LEVEL_MAXIMAL;
                break;

            default:
                ZP_INVALID_CODE_PATH();
                break;
        }

        FixedArray kCompilerOptions {
            slang::CompilerOptionEntry {
                .name = slang::CompilerOptionName::DebugInformation,
                .value { .kind = slang::CompilerOptionValueKind::Int, .intValue0 = (int)debugInfoLevel },
            },
            slang::CompilerOptionEntry {
                .name = slang::CompilerOptionName::DebugInformationFormat,
                .value { .kind = slang::CompilerOptionValueKind::Int, .intValue0 = (int)debugInfoFormat },
            },
            slang::CompilerOptionEntry {
                .name = slang::CompilerOptionName::Optimization,
                .value { .kind = slang::CompilerOptionValueKind::Int, .intValue0 = (int)optimizationLevel },
            },
        };

        const SlangCompileTarget compileTarget = SLANG_HLSL;
        const SlangProfileID profileId = g_globalSession->findProfile( "sm_6_0" );
        const SlangTargetFlags targetFlags = SLANG_TARGET_FLAG_GENERATE_WHOLE_PROGRAM;

        const FixedArray kTargets {
            slang::TargetDesc {
                .format = compileTarget,
                .profile = profileId,
                .flags = targetFlags,
                .floatingPointMode = SLANG_FLOATING_POINT_MODE_FAST,
                .compilerOptionEntries = kCompilerOptions.data(),
                .compilerOptionEntryCount = kCompilerOptions.length(),
            }
        };
        const FixedArray kIncludePaths {
            ".",
            "..",
        };
        const FixedArray kMacros {
            slang::PreprocessorMacroDesc {
                .name = "ZP_GPU_SHADER",
                .value = kOne
            },
        };

        const slang::SessionDesc sessionDesc {
            .targets = kTargets.data(),
            .targetCount = kTargets.length(),
            .searchPaths = kIncludePaths.data(),
            .searchPathCount = kIncludePaths.length(),
            .preprocessorMacros = kMacros.data(),
            .preprocessorMacroCount = kMacros.length(),
        };

        Slang::ComPtr<slang::ISession> session;
        r = g_globalSession->createSession( sessionDesc, session.writeRef() );
        ZP_ASSERT( SLANG_SUCCEEDED( r ) );

        const FixedArray kShaderStage
        {
            SLANG_STAGE_VERTEX,
            SLANG_STAGE_HULL,
            SLANG_STAGE_DOMAIN,
            SLANG_STAGE_GEOMETRY,
            SLANG_STAGE_FRAGMENT,
            SLANG_STAGE_COMPUTE,
            SLANG_STAGE_AMPLIFICATION,
            SLANG_STAGE_MESH,
        };
        ZP_STATIC_ASSERT( kShaderStage.length() == ShaderStage_Count );

        const FixedArray kDefaultEntryPointNames
        {
            "VertexMain",
            "HullMain",
            "DomainMain",
            "GeometryMain",
            "FragmentMain",
            "ComputeMain",
            "AmplificationMain",
            "MeshMain",
        };
        ZP_STATIC_ASSERT( kDefaultEntryPointNames.length() == ShaderStage_Count );

        Slang::ComPtr<slang::IBlob> diag;

        const char* srcCode = kRasterSampleSlang;

        Slang::ComPtr<slang::IModule> module;
        module = session->loadModuleFromSourceString( "simple", "simple.slang", srcCode, diag.writeRef() );
        ZP_ASSERT( module );

        LogDiagError( diag );

        FixedVector<slang::IComponentType*, 4> components;
        components.pushBack( module );

        MutableFixedString64 filename;

        const zp_int32_t entryPointCount = module->getDefinedEntryPointCount();
        for( zp_int32_t e = 0, emax = entryPointCount; e < emax; ++e )
        {
            Slang::ComPtr<slang::IEntryPoint> entryPoint;
            r = module->getDefinedEntryPoint( e, entryPoint.writeRef() );
            ZP_ASSERT( SLANG_SUCCEEDED( r ) );

            components.pushBack( entryPoint );

            Log::info() << "EntryPoint -> " << entryPoint->getFunctionReflection()->getName() << Log::endl;
        }

        //module->writeToFile( "module.txt" );

        Slang::ComPtr<slang::IComponentType> program;
        r = session->createCompositeComponentType( components.data(), components.length(), program.writeRef() );
        ZP_ASSERT( SLANG_SUCCEEDED( r ) );

        slang::ProgramLayout* programLayout;
        programLayout = program->getLayout( 0, diag.writeRef() );
        ZP_ASSERT( programLayout );

        LogDiagError( diag );

        ConvertReflectionToShader( programLayout );

        FixedArray linkOptions {
            slang::CompilerOptionEntry {
                .name = slang::CompilerOptionName::MacroDefine,
                .value {
                    .stringValue0 = "ZP_GPU_SHADER",
                    .stringValue1 = kOne,
                }
            }
        };
        Slang::ComPtr<slang::IComponentType> linkedProgram;
        //r = program->linkWithOptions( linkedProgram.writeRef(), linkOptions.length(), linkOptions.data(), diag.writeRef() );
        r = program->link( linkedProgram.writeRef(), diag.writeRef() );
        ZP_ASSERT( SLANG_SUCCEEDED( r ) );

        LogDiagError( diag );

        Slang::ComPtr<slang::IBlob> entryPointCode;

        for( zp_int32_t c = 0; c < entryPointCount; ++c)
        {
            r = linkedProgram->getEntryPointCode( c, 0, entryPointCode.writeRef(), diag.writeRef() );
            ZP_ASSERT( SLANG_SUCCEEDED( r ) );

            LogDiagError( diag );

            filename.clear();
            filename.appendFormat( "%d", c );
            filename.append( "-simple.txt" );
            DumpToFile( filename.c_str(), entryPointCode );
        }

        LogDiagError( diag );
    }
#if 0
    void ShaderCompilerExecuteJob( const JobHandle& parentJob, AssetCompilerTask* task )
    {
        ShaderTaskData* data = task->taskMemory.as<ShaderTaskData>();

        const zp_bool_t useJobSystem = false;

        zp_uint64_t processedShaderFeatureCount = 0;

        Vector<CompileJob> compileJobs( 64, task->memoryLabel );

        // TODO: invert loop so each shader feature can be grouped into tuple of shader programs,
        //  that way shader feature set "A" = ["vs 1", "ps 1"], "B" = ["vs 1", "ps 2"] etc
        //  then the tuple can be written out to the archive only keeping the unique programs
        for( zp_size_t s = 0; s < data->shaderFeatures.length(); ++s )
        {
            const ShaderFeature& shaderFeature = data->shaderFeatures[ s ];

            for( zp_uint32_t shaderProgram = 0; shaderProgram < ShaderProgramType_Count; ++shaderProgram )
            {
                const zp_uint32_t shaderProgramType = 1 << shaderProgram;

                // check shader feature is supported for the program type
                zp_bool_t shaderFeatureSupported = ( shaderFeature.shaderProgramSupportedType & shaderProgramType ) == shaderProgramType;

                // check for invalid shader features
                if( shaderFeatureSupported )
                {
                    for( const ShaderFeature& invalidShaderFeature : data->invalidShaderFeatures )
                    {
                        if( invalidShaderFeature.shaderFeatureHash == shaderFeature.shaderFeatureHash )
                        {
                            shaderFeatureSupported = false;
                            break;
                        }
                    }
                }

                // compile each valid shader feature
                if( shaderFeatureSupported )
                {
                    if( ( data->shaderCompilerSupportedTypes & shaderProgramType ) == shaderProgramType )
                    {
                        for( zp_size_t i = 0; i < shaderFeature.shaderFeatureCount; ++i )
                        {
                            ShaderFeature activeShaderFeature {
                                .shaderFeatureCount = 1,
                                .shaderProgramSupportedType = shaderFeature.shaderProgramSupportedType,
                            };
                            activeShaderFeature.shaderFeatures[ 0 ] = shaderFeature.shaderFeatures[ i ];

                            activeShaderFeature.shaderFeatureHash = zp_fnv64_1a( activeShaderFeature.shaderFeatures );

                            CompileJob job {
                                .task = task,
                                .shaderProgram = (ShaderProgramType)shaderProgram,
                                .shaderFeature = activeShaderFeature,
                                .entryPoint = data->entryPoints[ shaderProgram ]
                            };

                            compileJobs.pushBack( job );
                        }
                    }
                }
            }
        }

        JobHandle parentCompileJob {}; //= JobSystem::Start().schedule();

        // compile each job
        for( CompileJob& job : compileJobs )
        {
            if( useJobSystem )
            {
                // TODO: implement using new job system
                //JobSystem::Start( CompileShaderDXC, job, parentCompileJob ).schedule();
            }
            else
            {
                CompileShaderDXC( parentCompileJob, &job );
            }
        }

        JobSystem::ScheduleBatchJobs();
        JobSystem::Complete( parentCompileJob );

        char featureHashStr[16 + 1];

        // combine output shaders into single file
        ArchiveBuilder shaderArchiveBuilder( task->memoryLabel );
        // TODO: maybe load the existing archive? could be cool to merge the old and new, but could cause issues

        for( const CompileJob& job : compileJobs )
        {
            //zp_try_hash64_to_string( job.resultShaderFeatureHash, featureHashStr );
            //shaderArchiveBuilder.addBlock(  );

            // TODO: write out shader feature tuple ["vs 1", "ps 1"], ["vs 1", "ps 2"], etc.
            //  using the shader feature as the key
            // TODO: write of shader programs ["vs 1", "vs 2", ..., "ps 1", "ps 2",...]
            //  using the program name as the key
        }

        // compile archive
        DataStreamWriter shaderDataStream( task->memoryLabel );
        auto r = shaderArchiveBuilder.compile( 0, shaderDataStream );

        // write out compiled file on success
        if( r == ArchiveBuilderResult::Success )
        {
            zp_printfln( "Compile Success: %s", task->dstFile.c_str() );

            const Memory shaderDataMemory = shaderDataStream.memory();

            FileHandle dstFileHandle = Platform::OpenFileHandle( task->dstFile.c_str(), ZP_OPEN_FILE_MODE_WRITE );
            Platform::WriteFile( dstFileHandle, shaderDataMemory.ptr, shaderDataMemory.size );
            Platform::CloseFileHandle( dstFileHandle );
        }
        else
        {
            zp_printfln( "Failed to compile shader data %s", task->dstFile.c_str() );
        }
    }

#if 0
    void ShaderCompilerExecute( AssetCompilerTask* task )
    {
        // Old code for reference
        DxcBuffer sourceCode {
            .Ptr = data->shaderSource.ptr,
            .Size = data->shaderSource.size,
            .Encoding = 0,
        };

        HRESULT hr;

        AllocString srcFile = task->srcFile;
        AllocString dstFile = task->dstFile;

        LocalMalloc malloc( task->memoryLabel );

        for( zp_uint32_t shaderType = 0; shaderType < ShaderProgramType_Count; ++shaderType )
        {
            if( data->shaderCompilerSupportedTypes & ( 1 << shaderType ) )
            {
                ShaderModel sm = kShaderModelTypes[ data->shaderModel ];

                char entryPoint[kEntryPointSize];
                if( data->entryPoints[ shaderType ].empty() )
                {
                    auto start = zp_strrchr( srcFile.c_str(), '\\' ) + 1;
                    auto end = zp_strrchr( srcFile.c_str(), '.' );

                    zp_snprintf( entryPoint, "%.*s%s", end - start, start, kShaderEntryName[ shaderType ] );
                }
                else
                {
                    zp_strcpy( entryPoint, ZP_ARRAY_SIZE( entryPoint ), data->entryPoints[ shaderType ].c_str() );
                }

                WCHAR targetProfiler[] {
                    kShaderTypes[ shaderType ][ 0 ],
                    kShaderTypes[ shaderType ][ 1 ],
                    L'_',
                    static_cast<wchar_t>('0' + sm.major),
                    L'_',
                    static_cast<wchar_t>('0' + sm.minor),
                    '\0'
                };

                Vector<LPCWSTR> arguments( 24, task->memoryLabel );
                if( data->debug )
                {
                    arguments.pushBack( DXC_ARG_DEBUG );
                    arguments.pushBack( DXC_ARG_SKIP_OPTIMIZATIONS );
                }
                else
                {
                    arguments.pushBack( DXC_ARG_OPTIMIZATION_LEVEL3 );
                }

                arguments.pushBack( DXC_ARG_WARNINGS_ARE_ERRORS );

                //arguments.pushBack( L"-remove-unused-functions" );
                //arguments.pushBack( L"-remove-unused-globals" );

                arguments.pushBack( DXC_ARG_DEBUG_NAME_FOR_BINARY );

                //arguments.pushBack( L"-P" );

                arguments.pushBack( L"-ftime-report" );
                arguments.pushBack( L"-ftime-trace" );

                arguments.pushBack( L"-Qstrip_debug" );
                //arguments.pushBack( L"-Qstrip_priv" );
                //arguments.pushBack( L"-Qstrip_reflect" );
                //arguments.pushBack( L"-Qstrip_rootsignature" );

                arguments.pushBack( L"-I" );
                arguments.pushBack( L"Assets/ShaderLibrary/" );

                switch( data->shaderOutput )
                {
                    case SHADER_OUTPUT_DXIL:
                        break;

                    case SHADER_OUTPUT_SPIRV:
                        arguments.pushBack( L"-fspv-entrypoint-name=main" );
                        arguments.pushBack( L"-fspv-reflect" );
                        arguments.pushBack( L"-spirv" );
                        break;
                }

                Vector<DxcDefine> defines( 4, 0 );
                switch( data->shaderAPI )
                {
                    case SHADER_API_D3D:
                        defines.pushBack( { .Name= L"SHADER_API_D3D", .Value = L"1" } );
                        break;
                    case SHADER_API_VULKAN:
                        defines.pushBack( { .Name= L"SHADER_API_VULKAN", .Value = L"1" } );
                        break;
                }

                if( data->debug )
                {
                    defines.pushBack( { .Name= L"DEBUG", .Value = L"1" } );
                }

                Ptr <IDxcUtils> utils;
                HR( DxcCreateInstance2( &malloc, CLSID_DxcUtils, IID_PPV_ARGS( &utils.ptr ) ) );

                Ptr <IDxcIncludeHandler> includeHandler;
                HR( utils->CreateDefaultIncludeHandler( &includeHandler.ptr ) );

                Ptr <IDxcCompiler3> compiler;
                HR( DxcCreateInstance( CLSID_DxcCompiler, IID_PPV_ARGS( &compiler.ptr ) ) );

                WCHAR srcFileName[512];
                ::MultiByteToWideChar( CP_UTF8, 0, task->srcFile.c_str(), -1, srcFileName, ZP_ARRAY_SIZE( srcFileName ) );

                WCHAR entryPointName[kEntryPointSize];
                ::MultiByteToWideChar( CP_UTF8, 0, entryPoint, -1, entryPointName, ZP_ARRAY_SIZE( entryPointName ) );

                Ptr <IDxcCompilerArgs> args;
                utils->BuildArguments( srcFileName, entryPointName, targetProfiler, arguments.begin(), arguments.size(), defines.begin(), defines.size(), &args.ptr );

                MutableFixedString<1024> argsBuffer;
                auto f = args->GetArguments();
                for( zp_size_t i = 0; i < args->GetCount(); ++i )
                {
                    argsBuffer.appendFormat( "%ls ", f[ i ] );
                }
                argsBuffer.append( '\n' );

                zp_printfln( "Args: %s", argsBuffer.c_str() );

                // compile
                Ptr <IDxcResult> result;
                HR( compiler->Compile( &sourceCode, args->GetArguments(), args->GetCount(), includeHandler.ptr, IID_PPV_ARGS( &result.ptr ) ) );

                // check status
                HRESULT status;
                HR( result->GetStatus( &status ) );

                // compile successful
                if( SUCCEEDED( status ) )
                {
                    // errors
                    if( result->HasOutput( DXC_OUT_ERRORS ) )
                    {
                        Ptr <IDxcBlobUtf8> errorMsgs;
                        HR( result->GetOutput( DXC_OUT_ERRORS, IID_PPV_ARGS( &errorMsgs.ptr ), nullptr ) );

                        if( errorMsgs.ptr && errorMsgs->GetStringLength() )
                        {
                            zp_error_printfln( "[ERROR] " " %s", errorMsgs->GetStringPointer() );
                        }
                    }

                    // time report
                    if( result->HasOutput( DXC_OUT_TIME_REPORT ) )
                    {
                        Ptr <IDxcBlobUtf8> timeReport;
                        HR( result->GetOutput( DXC_OUT_TIME_REPORT, IID_PPV_ARGS( &timeReport.ptr ), nullptr ) );

                        if( timeReport.ptr && timeReport->GetStringLength() )
                        {
                            zp_printfln( "[REPORT]  " " %s", timeReport->GetStringPointer() );
                        }
                    }

                    // time trace
                    if( result->HasOutput( DXC_OUT_TIME_TRACE ) )
                    {
                        Ptr <IDxcBlobUtf8> timeTrace;
                        HR( result->GetOutput( DXC_OUT_TIME_TRACE, IID_PPV_ARGS( &timeTrace.ptr ), nullptr ) );

                        if( timeTrace.ptr && timeTrace->GetStringLength() )
                        {
                            zp_printfln( "[TRACE]  " " %s", timeTrace->GetStringPointer() );
                        }
                    }

                    // pdb file and path
                    if( result->HasOutput( DXC_OUT_PDB ) )
                    {
                        Ptr <IDxcBlob> pdbData;
                        Ptr <IDxcBlobUtf16> pdbPathFromCompiler;
                        HR( result->GetOutput( DXC_OUT_PDB, IID_PPV_ARGS( &pdbData.ptr ), &pdbPathFromCompiler.ptr ) );

                        if( pdbData.ptr && pdbData->GetBufferSize() )
                        {
                            char pdbFilePath[512];
                            ::WideCharToMultiByte( CP_UTF8, 0, pdbPathFromCompiler->GetStringPointer(), -1, pdbFilePath, ZP_ARRAY_SIZE( pdbFilePath ), nullptr, nullptr );

                            zp_handle_t pdbFileHandle = Platform::OpenFileHandle( pdbFilePath, ZP_OPEN_FILE_MODE_WRITE );
                            Platform::WriteFile( pdbFileHandle, pdbData->GetBufferPointer(), pdbData->GetBufferSize() );
                            Platform::CloseFileHandle( pdbFileHandle );

                            zp_printfln( "[PDB]  " " %s", pdbFilePath );
                        }
                    }

                    // reflection
                    if( result->HasOutput( DXC_OUT_REFLECTION ) )
                    {
                        Ptr <IDxcBlob> reflection;
                        HR( result->GetOutput( DXC_OUT_REFLECTION, IID_PPV_ARGS( &reflection.ptr ), nullptr ) );

                        zp_printfln( "[REFLECTION]  " " " );

                        if( reflection.ptr )
                        {
                            // save to file also
                            DxcBuffer reflectionData {
                                .Ptr = reflection->GetBufferPointer(),
                                .Size = reflection->GetBufferSize(),
                                .Encoding = 0
                            };

                            Ptr <ID3D12ShaderReflection> shaderReflection;
                            HR( utils->CreateReflection( &reflectionData, IID_PPV_ARGS( &shaderReflection.ptr ) ) );

                            if( shaderReflection.ptr )
                            {
                                D3D12_SHADER_DESC shaderDesc;
                                HR( shaderReflection->GetDesc( &shaderDesc ) );

                                zp_printfln( "Constant Buffers: %d ", shaderDesc.ConstantBuffers );
                                for( zp_uint32_t i = 0; i < shaderDesc.ConstantBuffers; ++i )
                                {
                                    auto cb = shaderReflection->GetConstantBufferByIndex( i );

                                    D3D12_SHADER_BUFFER_DESC cbDesc;
                                    cb->GetDesc( &cbDesc );

                                    zp_printfln( "  %s %d %d %d", cbDesc.Name, cbDesc.Type, cbDesc.Variables, cbDesc.Size );
                                }

                                zp_printfln( "Bound Resources: %d ", shaderDesc.BoundResources );
                                for( zp_uint32_t i = 0; i < shaderDesc.BoundResources; ++i )
                                {
                                    D3D12_SHADER_INPUT_BIND_DESC inputBindDesc;
                                    HR( shaderReflection->GetResourceBindingDesc( i, &inputBindDesc ) );

                                    zp_printfln( "  %s ", inputBindDesc.Name );
                                }

                                zp_printfln( "Input Parameters: %d ", shaderDesc.InputParameters );
                                for( zp_uint32_t i = 0; i < shaderDesc.InputParameters; ++i )
                                {
                                    D3D12_SIGNATURE_PARAMETER_DESC parameterDesc;
                                    HR( shaderReflection->GetInputParameterDesc( i, &parameterDesc ) );

                                    zp_printfln( "  %s ", parameterDesc.SemanticName );
                                }

                                zp_printfln( "Output Parameters: %d ", shaderDesc.OutputParameters );
                                for( zp_uint32_t i = 0; i < shaderDesc.OutputParameters; ++i )
                                {
                                    D3D12_SIGNATURE_PARAMETER_DESC parameterDesc;
                                    HR( shaderReflection->GetOutputParameterDesc( i, &parameterDesc ) );

                                    zp_printfln( "  %d %s", parameterDesc.SystemValueType, parameterDesc.SemanticName );
                                }

#define INFO_D( n ) "%30s: %d", #n, shaderDesc.n
#define INFO_S( n ) "%30s: %s", #n, shaderDesc.n

                                zp_printfln( INFO_D( Version ) );
                                zp_printfln( INFO_S( Creator ) );
                                zp_printfln( INFO_D( ConstantBuffers ) );
                                zp_printfln( INFO_D( BoundResources ) );
                                zp_printfln( INFO_D( InputParameters ) );
                                zp_printfln( INFO_D( OutputParameters ) );
                                zp_printfln( INFO_D( InstructionCount ) );
                                zp_printfln( INFO_D( TempRegisterCount ) );
                                zp_printfln( INFO_D( TempArrayCount ) );
                                zp_printfln( INFO_D( DefCount ) );
                                zp_printfln( INFO_D( DclCount ) );
                                zp_printfln( INFO_D( TextureNormalInstructions ) );
                                zp_printfln( INFO_D( TextureLoadInstructions ) );
                                zp_printfln( INFO_D( TextureCompInstructions ) );
                                zp_printfln( INFO_D( TextureBiasInstructions ) );
                                zp_printfln( INFO_D( TextureGradientInstructions ) );
                                zp_printfln( INFO_D( FloatInstructionCount ) );
                                zp_printfln( INFO_D( IntInstructionCount ) );
                                zp_printfln( INFO_D( UintInstructionCount ) );
                                zp_printfln( INFO_D( StaticFlowControlCount ) );
                                zp_printfln( INFO_D( DynamicFlowControlCount ) );
                                zp_printfln( INFO_D( MacroInstructionCount ) );
                                zp_printfln( INFO_D( ArrayInstructionCount ) );
                                zp_printfln( INFO_D( CutInstructionCount ) );
                                zp_printfln( INFO_D( EmitInstructionCount ) );
                                zp_printfln( INFO_D( GSOutputTopology ) );
                                zp_printfln( INFO_D( GSMaxOutputVertexCount ) );
                                zp_printfln( INFO_D( InputPrimitive ) );
                                zp_printfln( INFO_D( PatchConstantParameters ) );
                                zp_printfln( INFO_D( cGSInstanceCount ) );
                                zp_printfln( INFO_D( cControlPoints ) );
                                zp_printfln( INFO_D( HSOutputPrimitive ) );
                                zp_printfln( INFO_D( HSPartitioning ) );
                                zp_printfln( INFO_D( TessellatorDomain ) );
                                zp_printfln( INFO_D( cBarrierInstructions ) );
                                zp_printfln( INFO_D( cInterlockedInstructions ) );
                                zp_printfln( INFO_D( cTextureStoreInstructions ) );
#undef INFO_D
#undef INFO_S
                            }
                        }
                    }

                    // shader hash
                    if( result->HasOutput( DXC_OUT_SHADER_HASH ) )
                    {
                        Ptr <IDxcBlob> shaderHash;
                        HR( result->GetOutput( DXC_OUT_SHADER_HASH, IID_PPV_ARGS( &shaderHash.ptr ), nullptr ) );

                        if( shaderHash.ptr && shaderHash->GetBufferSize() )
                        {
                            DxcShaderHash* dxcShaderHash = static_cast<DxcShaderHash*>(shaderHash->GetBufferPointer());

                            char buff[32 + 1];
                            for( int i = 0; i < 16; ++i )
                            {
                                zp_snprintf( buff + ( i * 2 ), 3, "%02x", dxcShaderHash->HashDigest[ i ] );
                            }
                            buff[ 32 ] = 0;
                            zp_printfln( "[HASH] " " Hash: %s", buff );
                        }
                    }

                    // text
                    if( result->HasOutput( DXC_OUT_TEXT ) )
                    {
                        Ptr <IDxcBlobUtf8> text;
                        HR( result->GetOutput( DXC_OUT_TEXT, IID_PPV_ARGS( &text.ptr ), nullptr ) );

                        if( text.ptr && text->GetStringLength() )
                        {
                            zp_printfln( "[TEXT] " " %s", text->GetStringPointer() );
                        }
                    }

                    // remarks
                    if( result->HasOutput( DXC_OUT_REMARKS ) )
                    {
                        Ptr <IDxcBlobUtf8> remarks;
                        HR( result->GetOutput( DXC_OUT_REMARKS, IID_PPV_ARGS( &remarks.ptr ), nullptr ) );

                        if( remarks.ptr && remarks->GetStringLength() )
                        {
                            zp_printfln( "[REMARKS] " " %s", remarks->GetStringPointer() );
                        }
                    }

                    // hlsl
                    if( result->HasOutput( DXC_OUT_HLSL ) )
                    {
                        Ptr <IDxcBlobUtf8> hlsl;
                        HR( result->GetOutput( DXC_OUT_HLSL, IID_PPV_ARGS( &hlsl.ptr ), nullptr ) );

                        if( hlsl.ptr && hlsl->GetStringLength() )
                        {
                            zp_printfln( "[HSLS] " " %s", hlsl->GetStringPointer() );
                        }
                    }

                    // disassembly
                    if( result->HasOutput( DXC_OUT_DISASSEMBLY ) )
                    {
                        Ptr <IDxcBlobUtf8> disassembly;
                        HR( result->GetOutput( DXC_OUT_DISASSEMBLY, IID_PPV_ARGS( &disassembly.ptr ), nullptr ) );

                        if( disassembly.ptr && disassembly->GetStringLength() )
                        {
                            zp_printfln( "[DISASSEMBLY] " " %s", disassembly->GetStringPointer() );
                        }
                    }

                    // disassembly
                    if( result->HasOutput( DXC_OUT_EXTRA_OUTPUTS ) )
                    {
                        Ptr <IDxcExtraOutputs> extraOutput;
                        HR( result->GetOutput( DXC_OUT_EXTRA_OUTPUTS, IID_PPV_ARGS( &extraOutput.ptr ), nullptr ) );

                        if( extraOutput.ptr && extraOutput->GetOutputCount() )
                        {
                            zp_printfln( "[EXTRA] " " %d", extraOutput->GetOutputCount() );
                        }
                    }

                    // shader byte code
                    if( result->HasOutput( DXC_OUT_OBJECT ) )
                    {
                        Ptr <IDxcBlob> shaderObj;
                        HR( result->GetOutput( DXC_OUT_OBJECT, IID_PPV_ARGS( &shaderObj.ptr ), nullptr ) );

                        zp_handle_t dstFileHandle = Platform::OpenFileHandle( dstFile.c_str(), ZP_OPEN_FILE_MODE_WRITE );
                        Platform::WriteFile( dstFileHandle, shaderObj->GetBufferPointer(), shaderObj->GetBufferSize() );
                        Platform::CloseFileHandle( dstFileHandle );

                        zp_printfln( "[OBJECT] " " %s", dstFile.c_str() );
                    }
                }
                else
                {
                    // errors
                    if( result->HasOutput( DXC_OUT_ERRORS ) )
                    {
                        Ptr <IDxcBlobUtf8> errorMsgs;
                        HR( result->GetOutput( DXC_OUT_ERRORS, IID_PPV_ARGS( &errorMsgs.ptr ), nullptr ) );

                        if( errorMsgs->GetStringLength() )
                        {
                            zp_error_printfln( "[ERROR] " " %s", errorMsgs->GetStringPointer() );
                        }
                    }

                    // text
                    if( result->HasOutput( DXC_OUT_TEXT ) )
                    {
                        Ptr <IDxcBlobUtf8> text;
                        HR( result->GetOutput( DXC_OUT_TEXT, IID_PPV_ARGS( &text.ptr ), nullptr ) );

                        if( text.ptr && text->GetStringLength() )
                        {
                            zp_printfln( "[TEXT]  " " %s", text->GetStringPointer() );
                        }
                    }

                    // hlsl
                    if( result->HasOutput( DXC_OUT_HLSL ) )
                    {
                        Ptr <IDxcBlobUtf8> hlsl;
                        HR( result->GetOutput( DXC_OUT_HLSL, IID_PPV_ARGS( &hlsl.ptr ), nullptr ) );

                        if( hlsl.ptr && hlsl->GetStringLength() )
                        {
                            zp_printfln( "[HLSL]  " " %s", hlsl->GetStringPointer() );
                        }
                    }

                    // remarks
                    if( result->HasOutput( DXC_OUT_REMARKS ) )
                    {
                        Ptr <IDxcBlobUtf8> remarks;
                        HR( result->GetOutput( DXC_OUT_REMARKS, IID_PPV_ARGS( &remarks.ptr ), nullptr ) );

                        if( remarks.ptr && remarks->GetStringLength() )
                        {
                            zp_printfln( "[REMARKS]  " " %s", remarks->GetStringPointer() );
                        }
                    }
                }
            }
        }

        //
        //
        //
        DataBuilder dataBuilder( 0 );

        dataBuilder.beginRecodring();

        dataBuilder.pushObject( String::As( "shader" ) );

        dataBuilder.popObject();

        dataBuilder.endRecording();
        //
        //
        //

        Memory compiledMemory = {};
        auto r = dataBuilder.compile( ZP_DATA_BUILDER_COMPILER_OPTION_NONE, compiledMemory );
        if( r == DataBuilderCompilerResult::Success )
        {

        }
    }
#endif

    Memory ShaderCompilerCreateTaskMemory( MemoryLabel memoryLabel, const String& inFile, const String& outFile, const CommandLine& cmdLine )
    {
        ShaderTaskData* data = nullptr;

        if( Platform::FileExists( inFile.c_str() ) )
        {
            data = ZP_NEW( memoryLabel, ShaderTaskData );

            // default values
            data->debug = false;
            data->shaderModel = SHADER_MODEL_TYPE_6_0;
            data->shaderCompilerSupportedTypes = 0;

            data->shaderAPI = SHADER_API_D3D;
            data->shaderOutput = SHADER_OUTPUT_DXIL;

            data->shaderSource = {};

            // add "empty" feature
            data->allShaderFeatures.pushBack( String::As( "_" ) );

            String ext = String::As( zp_strrchr( inFile.c_str(), '.' ) );
            zp_uint32_t requiredShaderPrograms = 0;

            if( zp_strcmp( ext, ".shader" ) == 0 )
            {
                requiredShaderPrograms = SHADER_PROGRAM_SUPPORTED_TYPE_VERTEX | SHADER_PROGRAM_SUPPORTED_TYPE_FRAGMENT;
            }
            else if( zp_strcmp( ext, ".compute" ) == 0 )
            {
                requiredShaderPrograms = SHADER_PROGRAM_SUPPORTED_TYPE_COMPUTE;
            }
            else
            {
                ZP_INVALID_CODE_PATH_MSG( "Unsupported shader file type used" );
            }

            FileHandle inFileHandle = Platform::OpenFileHandle( inFile.c_str(), ZP_OPEN_FILE_MODE_READ );
            const zp_size_t inFileSize = Platform::GetFileSize( inFileHandle );
            void* inFileData = ZP_MALLOC( memoryLabel, inFileSize );

            zp_size_t inFileReadSize = Platform::ReadFile( inFileHandle, inFileData, inFileSize );
            ZP_ASSERT( inFileSize == inFileReadSize );
            Platform::CloseFileHandle( inFileHandle );

            data->shaderSource.set( memoryLabel, (zp_char8_t*)inFileData, inFileSize ); //= { .str = (zp_char8_t*)inFileData, .length = inFileSize };

            // TODO: move to helper functions
            Tokenizer lineTokenizer( data->shaderSource.asString(), "\r\n" );

            String line {};
            while( lineTokenizer.next( line ) )
            {
                if( zp_strnstr( line, "#pragma " ) == line.c_str() )
                {
                    Tokenizer pragmaTokenizer( line, " " );

                    String pragma {};
                    while( pragmaTokenizer.next( pragma ) )
                    {
                        if( zp_strcmp( pragma, "vertex" ) == 0 )
                        {
                            data->shaderCompilerSupportedTypes |= SHADER_PROGRAM_SUPPORTED_TYPE_VERTEX;
                            data->entryPoints[ SHADER_PROGRAM_TYPE_VERTEX ] = String::As( "" );

                            String pragmaOp {};
                            if( pragmaTokenizer.next( pragmaOp ) )
                            {
                                data->entryPoints[ SHADER_PROGRAM_TYPE_VERTEX ] = pragmaOp;
                            }

                            zp_printfln( "Vertex Entry - %.*s", data->entryPoints[ SHADER_PROGRAM_TYPE_VERTEX ].length(), data->entryPoints[ SHADER_PROGRAM_TYPE_VERTEX ].c_str() );
                        }
                        else if( zp_strcmp( pragma, "fragment" ) == 0 )
                        {
                            data->shaderCompilerSupportedTypes |= SHADER_PROGRAM_SUPPORTED_TYPE_FRAGMENT;
                            data->entryPoints[ SHADER_PROGRAM_TYPE_FRAGMENT ] = String::As( "" );

                            String pragmaOp {};
                            if( pragmaTokenizer.next( pragmaOp ) )
                            {
                                data->entryPoints[ SHADER_PROGRAM_TYPE_FRAGMENT ] = pragmaOp;
                            }

                            zp_printfln( "Fragment Entry - %.*s", data->entryPoints[ SHADER_PROGRAM_TYPE_FRAGMENT ].length(), data->entryPoints[ SHADER_PROGRAM_TYPE_FRAGMENT ].c_str() );
                        }
                        else if( zp_strcmp( pragma, "kernel" ) == 0 )
                        {
                            data->shaderCompilerSupportedTypes |= SHADER_PROGRAM_SUPPORTED_TYPE_COMPUTE;
                            data->entryPoints[ SHADER_PROGRAM_TYPE_COMPUTE ] = String::As( "" );

                            String pragmaOp {};
                            if( pragmaTokenizer.next( pragmaOp ) )
                            {
                                data->entryPoints[ SHADER_PROGRAM_TYPE_COMPUTE ] = pragmaOp;
                            }

                            zp_printfln( "Compute Entry - %.*s", data->entryPoints[ SHADER_PROGRAM_TYPE_COMPUTE ].length(), data->entryPoints[ SHADER_PROGRAM_TYPE_COMPUTE ].c_str() );
                        }
                        else if( zp_strcmp( pragma, "enable_debug" ) == 0 )
                        {
                            data->debug = true;
                            zp_printfln( "Use Debug - true" );
                        }
                        else if( zp_strcmp( pragma, "target" ) == 0 )
                        {
                            String pragmaOp {};
                            if( pragmaTokenizer.next( pragmaOp ) )
                            {
                                // format "#.#"
                                if( pragmaOp.length() == 3 && pragmaOp.str()[ 1 ] == '.' )
                                {
                                    ShaderModel model {};

                                    zp_bool_t ok = false;
                                    ok |= zp_try_parse_uint8( pragmaOp.c_str() + 0, 1, &model.major );
                                    ok |= zp_try_parse_uint8( pragmaOp.c_str() + 2, 1, &model.minor );
                                    if( ok )
                                    {
                                        zp_size_t index;
                                        if( zp_try_find_index( kShaderModelTypes, model, []( const ShaderModel& x, const ShaderModel& y )
                                        {
                                            return x.major == y.major && x.minor == y.minor;
                                        }, index ) )
                                        {
                                            data->shaderModel = static_cast<ShaderModelType>( index );

                                            zp_printfln( "Target - Major: %d Minor %d  Model Index %d", model.major, model.minor, data->shaderModel );
                                        }
                                    }
                                }
                            }
                        }
                            // TODO: have dxc precompile hlsl and then parse pragmas from that to support pragmas in included files, then compile variants from the preprocessed
                        else if( zp_strnstr( pragma, "shader_feature" ) != nullptr )
                        {
                            zp_uint32_t programSupportedType = SHADER_PROGRAM_SUPPORTED_TYPE_NONE;

                            if( zp_strcmp( pragma, "shader_feature_vertex" ) == 0 )
                            {
                                programSupportedType |= SHADER_PROGRAM_SUPPORTED_TYPE_VERTEX;
                            }
                            else if( zp_strcmp( pragma, "shader_feature_fragment" ) == 0 )
                            {
                                programSupportedType |= SHADER_PROGRAM_SUPPORTED_TYPE_FRAGMENT;
                            }
                            else if( zp_strcmp( pragma, "shader_feature" ) == 0 )
                            {
                                programSupportedType |= SHADER_PROGRAM_SUPPORTED_TYPE_VERTEX;
                                programSupportedType |= SHADER_PROGRAM_SUPPORTED_TYPE_FRAGMENT;
                            }

                            if( programSupportedType != 0 )
                            {
                                MutableFixedString<ShaderProgramType_Count> pp;
                                if( programSupportedType & SHADER_PROGRAM_SUPPORTED_TYPE_VERTEX )
                                {
                                    pp.append( "V" );
                                }

                                if( programSupportedType & SHADER_PROGRAM_SUPPORTED_TYPE_FRAGMENT )
                                {
                                    pp.append( "F" );
                                }

                                if( programSupportedType & SHADER_PROGRAM_SUPPORTED_TYPE_GEOMETRY )
                                {
                                    pp.append( "G" );
                                }

                                if( programSupportedType & SHADER_PROGRAM_SUPPORTED_TYPE_TESSELATION )
                                {
                                    pp.append( "T" );
                                }

                                if( programSupportedType & SHADER_PROGRAM_SUPPORTED_TYPE_HULL )
                                {
                                    pp.append( "H" );
                                }

                                if( programSupportedType & SHADER_PROGRAM_SUPPORTED_TYPE_DOMAIN )
                                {
                                    pp.append( "D" );
                                }

                                if( programSupportedType & SHADER_PROGRAM_SUPPORTED_TYPE_COMPUTE )
                                {
                                    pp.append( "C" );
                                }

                                if( programSupportedType & SHADER_PROGRAM_SUPPORTED_TYPE_MESH )
                                {
                                    pp.append( "M" );
                                }

                                if( programSupportedType & SHADER_PROGRAM_SUPPORTED_TYPE_TASK )
                                {
                                    pp.append( "A" );
                                }

                                zp_printfln( "Shader Programs %s:", pp.c_str() );

                                ShaderFeature shaderFeature { .shaderProgramSupportedType = programSupportedType };

                                // parse each feature
                                String pragmaOp {};
                                while( pragmaTokenizer.next( pragmaOp ) )
                                {
                                    ZP_ASSERT( shaderFeature.shaderFeatureCount < shaderFeature.shaderFeatures.length() );
                                    zp_printfln( "  - %.*s", pragmaOp.length(), pragmaOp.c_str() );

                                    // build feature list, add new one if it doesn't exist
                                    zp_size_t index = data->allShaderFeatures.findIndexOf( [&pragmaOp]( const String& x )
                                    {
                                        return zp_strcmp( x, pragmaOp ) == 0;
                                    } );

                                    if( index == zp::npos )
                                    {
                                        index = data->allShaderFeatures.length();
                                        data->allShaderFeatures.pushBack( pragmaOp );
                                    }

                                    shaderFeature.shaderFeatures[ shaderFeature.shaderFeatureCount++ ] = index;
                                }

                                // sort feature set in descending order
                                zp_qsort3( shaderFeature.shaderFeatures.begin(), shaderFeature.shaderFeatures.end(), zp_cmp_dsc<zp_size_t> );

                                // generate hash from sorted list
                                shaderFeature.shaderFeatureHash = zp_fnv64_1a( shaderFeature.shaderFeatures );

                                data->shaderFeatures.pushBack( shaderFeature );
                            }
                            break;
                        }
                        else if( zp_strnstr( pragma, "invalid_shader_feature" ) != nullptr )
                        {
                            zp_printfln( "Invalid Shader Features:" );

                            ShaderFeature shaderFeature { .shaderProgramSupportedType = SHADER_PROGRAM_SUPPORTED_TYPE_ALL };

                            // parse each feature
                            String pragmaOp {};
                            while( pragmaTokenizer.next( pragmaOp ) )
                            {
                                ZP_ASSERT( shaderFeature.shaderFeatureCount < shaderFeature.shaderFeatures.length() );
                                zp_printfln( "  - %.*s", pragmaOp.length(), pragmaOp.c_str() );

                                // build feature list, add new one if it doesn't exist
                                zp_size_t index = data->allShaderFeatures.findIndexOf( [&pragmaOp]( const String& x )
                                {
                                    return zp_strcmp( x, pragmaOp ) == 0;
                                } );

                                if( index == zp::npos )
                                {
                                    index = data->allShaderFeatures.length();
                                    data->allShaderFeatures.pushBack( pragmaOp );
                                }

                                shaderFeature.shaderFeatures[ shaderFeature.shaderFeatureCount++ ] = index;
                            }

                            // sort feature set
                            zp_qsort3( shaderFeature.shaderFeatures.begin(), shaderFeature.shaderFeatures.end(), zp_cmp_dsc<zp_size_t> );

                            // generate hash from sorted list
                            shaderFeature.shaderFeatureHash = zp_fnv64_1a( shaderFeature.shaderFeatures );

                            data->invalidShaderFeatures.pushBack( shaderFeature );
                        }
                    }
                }
            }

            ZP_ASSERT( ( data->shaderCompilerSupportedTypes & requiredShaderPrograms ) == requiredShaderPrograms );
        }

        return { .ptr = data, .size = data ? sizeof( ShaderTaskData ) : 0 };
    }

    void ShaderCompilerDestroyTaskMemory( MemoryLabel memoryLabel, Memory memory )
    {
        ZP_SAFE_DELETE( ShaderTaskData, memory.ptr );
    }

    void ShaderCompilerSlangExecute()
    {
    }
#endif
}
