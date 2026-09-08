/*
  ==============================================================================

   This file is part of the JUCE framework.
   Copyright (c) Raw Material Software Limited

   JUCE is an open source framework subject to commercial or open source
   licensing.

   By downloading, installing, or using the JUCE framework, or combining the
   JUCE framework with any other source code, object code, content or any other
   copyrightable work, you agree to the terms of the JUCE End User Licence
   Agreement, and all incorporated terms including the JUCE Privacy Policy and
   the JUCE Website Terms of Service, as applicable, which will bind you. If you
   do not agree to the terms of these agreements, we will not license the JUCE
   framework to you, and you must discontinue the installation or download
   process and cease use of the JUCE framework.

   JUCE End User Licence Agreement: https://juce.com/legal/juce-9-licence/
   JUCE Privacy Policy: https://juce.com/juce-privacy-policy
   JUCE Website Terms of Service: https://juce.com/juce-website-terms-of-service/

   Or:

   You may also use this code under the terms of the AGPLv3:
   https://www.gnu.org/licenses/agpl-3.0.en.html

   THE JUCE FRAMEWORK IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL
   WARRANTIES, WHETHER EXPRESSED OR IMPLIED, INCLUDING WARRANTY OF
   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

// This file implements a native PipeWire audio device type. PipeWire is loaded
// dynamically at run-time (like the JACK backend does), so apps which enable
// JUCE_PIPEWIRE don't need to link against libpipewire.

#ifndef JUCE_PIPEWIRE_LOGGING
 #define JUCE_PIPEWIRE_LOGGING JUCE_DEBUG
#endif

#if JUCE_PIPEWIRE_LOGGING
 #define JUCE_PIPEWIRE_LOG(dbgtext)  { juce::String tempDbgBuf ("PipeWire: "); tempDbgBuf << dbgtext; Logger::writeToLog (tempDbgBuf); DBG (tempDbgBuf); }
#else
 #define JUCE_PIPEWIRE_LOG(dbgtext)
#endif

//==============================================================================
static void* juce_libpipewireHandle = nullptr;

static void* juce_loadPipeWireFunction (const char* const name)
{
    if (juce_libpipewireHandle == nullptr)
        return nullptr;

    return dlsym (juce_libpipewireHandle, name);
}

static bool loadPipeWireLibrary()
{
    if (juce_libpipewireHandle == nullptr)
    {
        juce_libpipewireHandle = dlopen ("libpipewire-0.3.so.0", RTLD_LAZY);

        if (juce_libpipewireHandle == nullptr)
            juce_libpipewireHandle = dlopen ("libpipewire-0.3.so", RTLD_LAZY);
    }

    return juce_libpipewireHandle != nullptr;
}

#define JUCE_DECL_PIPEWIRE_FUNCTION(return_type, fn_name, argument_types, arguments)  \
  static inline return_type fn_name argument_types                                    \
  {                                                                                   \
      using ReturnType = return_type;                                                 \
      typedef return_type (*fn_type) argument_types;                                  \
      static fn_type fn = (fn_type) juce_loadPipeWireFunction (#fn_name);             \
      jassert (fn != nullptr);                                                        \
      return (fn != nullptr) ? ((*fn) arguments) : ReturnType();                      \
  }

#define JUCE_DECL_VOID_PIPEWIRE_FUNCTION(fn_name, argument_types, arguments)          \
  static inline void fn_name argument_types                                           \
  {                                                                                   \
      typedef void (*fn_type) argument_types;                                         \
      static fn_type fn = (fn_type) juce_loadPipeWireFunction (#fn_name);             \
      jassert (fn != nullptr);                                                        \
      if (fn != nullptr) (*fn) arguments;                                             \
  }

//==============================================================================
// Core/context/loop
JUCE_DECL_VOID_PIPEWIRE_FUNCTION (pw_init, (int* argc, char** argv), (argc, argv))
JUCE_DECL_PIPEWIRE_FUNCTION (struct pw_main_loop*, pw_main_loop_new, (const struct spa_dict* props), (props))
JUCE_DECL_VOID_PIPEWIRE_FUNCTION (pw_main_loop_destroy, (struct pw_main_loop* loop), (loop))
JUCE_DECL_PIPEWIRE_FUNCTION (struct pw_loop*, pw_main_loop_get_loop, (struct pw_main_loop* loop), (loop))
JUCE_DECL_VOID_PIPEWIRE_FUNCTION (pw_main_loop_run, (struct pw_main_loop* loop), (loop))
JUCE_DECL_VOID_PIPEWIRE_FUNCTION (pw_main_loop_quit, (struct pw_main_loop* loop), (loop))
JUCE_DECL_PIPEWIRE_FUNCTION (struct pw_loop*, pw_loop_new, (const struct spa_dict* props), (props))
JUCE_DECL_VOID_PIPEWIRE_FUNCTION (pw_loop_destroy, (struct pw_loop* loop), (loop))
JUCE_DECL_PIPEWIRE_FUNCTION (int, pw_loop_iterate, (struct pw_loop* loop, int timeout), (loop, timeout))
JUCE_DECL_PIPEWIRE_FUNCTION (struct pw_context*, pw_context_new, (struct pw_loop* loop, struct pw_properties* props, size_t user_data_size), (loop, props, user_data_size))
JUCE_DECL_VOID_PIPEWIRE_FUNCTION (pw_context_destroy, (struct pw_context* context), (context))
JUCE_DECL_PIPEWIRE_FUNCTION (struct pw_core*, pw_context_connect, (struct pw_context* context, struct pw_properties* properties, size_t user_data_size), (context, properties, user_data_size))
JUCE_DECL_VOID_PIPEWIRE_FUNCTION (pw_core_disconnect, (struct pw_core* core), (core))
JUCE_DECL_PIPEWIRE_FUNCTION (int, pw_core_sync, (struct pw_core* core, uint32_t id, int seq), (core, id, seq))
JUCE_DECL_VOID_PIPEWIRE_FUNCTION (pw_core_add_listener, (struct pw_core* core, struct spa_hook* listener, const struct pw_core_events* events, void* data), (core, listener, events, data))

// Registry / proxies
JUCE_DECL_PIPEWIRE_FUNCTION (struct pw_registry*, pw_core_get_registry, (struct pw_core* core, uint32_t version, size_t user_data_size), (core, version, user_data_size))
JUCE_DECL_VOID_PIPEWIRE_FUNCTION (pw_registry_add_listener, (struct pw_registry* registry, struct spa_hook* listener, const struct pw_registry_events* events, void* data), (registry, listener, events, data))
JUCE_DECL_PIPEWIRE_FUNCTION (struct pw_proxy*, pw_registry_bind, (struct pw_registry* registry, uint32_t id, const char* type, uint32_t version, size_t user_data_size), (registry, id, type, version, user_data_size))
JUCE_DECL_VOID_PIPEWIRE_FUNCTION (pw_proxy_destroy, (struct pw_proxy* proxy), (proxy))
JUCE_DECL_VOID_PIPEWIRE_FUNCTION (pw_proxy_add_object_listener, (struct pw_proxy* proxy, struct spa_hook* listener, const void* funcs, void* data), (proxy, listener, funcs, data))

// Streams
JUCE_DECL_PIPEWIRE_FUNCTION (struct pw_stream*, pw_stream_new, (struct pw_core* core, const char* name, struct pw_properties* props), (core, name, props))
JUCE_DECL_VOID_PIPEWIRE_FUNCTION (pw_stream_destroy, (struct pw_stream* stream), (stream))
JUCE_DECL_VOID_PIPEWIRE_FUNCTION (pw_stream_add_listener, (struct pw_stream* stream, struct spa_hook* listener, const struct pw_stream_events* events, void* data), (stream, listener, events, data))
JUCE_DECL_PIPEWIRE_FUNCTION (int, pw_stream_connect, (struct pw_stream* stream, enum pw_direction direction, uint32_t target_id, enum pw_stream_flags flags, const struct spa_pod** params, uint32_t n_params), (stream, direction, target_id, flags, params, n_params))
JUCE_DECL_VOID_PIPEWIRE_FUNCTION (pw_stream_disconnect, (struct pw_stream* stream), (stream))
JUCE_DECL_PIPEWIRE_FUNCTION (struct pw_buffer*, pw_stream_dequeue_buffer, (struct pw_stream* stream), (stream))
JUCE_DECL_VOID_PIPEWIRE_FUNCTION (pw_stream_queue_buffer, (struct pw_stream* stream, struct pw_buffer* buffer), (stream, buffer))
JUCE_DECL_PIPEWIRE_FUNCTION (int, pw_stream_set_active, (struct pw_stream* stream, bool active), (stream, active))
JUCE_DECL_PIPEWIRE_FUNCTION (int, pw_stream_get_time, (struct pw_stream* stream, struct pw_time* time), (stream, time))

// Properties
JUCE_DECL_PIPEWIRE_FUNCTION (int, pw_properties_set, (struct pw_properties* properties, const char* key, const char* value), (properties, key, value))
JUCE_DECL_VOID_PIPEWIRE_FUNCTION (pw_properties_free, (struct pw_properties* properties), (properties))

// pw_properties_new() is a variadic function, so it can't be declared with the
// macro above. We only ever need an empty property bag which we fill in with
// pw_properties_set() afterwards.
static inline struct pw_properties* createEmptyPipeWireProperties()
{
    typedef struct pw_properties* (*fn_type) (const char*, ...);
    static fn_type fn = (fn_type) juce_loadPipeWireFunction ("pw_properties_new");
    jassert (fn != nullptr);

    if (fn == nullptr)
        return nullptr;

    return (*fn) ((const char*) nullptr);
}

//==============================================================================
static const char* getProp (const struct spa_dict* props, const char* key)
{
    if (props == nullptr)
        return nullptr;

    return spa_dict_lookup (props, key);
}

static bool mediaClassIsSink (const char* mediaClass)
{
    return mediaClass != nullptr && String (mediaClass).startsWith ("Audio/Sink");
}

static bool mediaClassIsSource (const char* mediaClass)
{
    return mediaClass != nullptr && String (mediaClass).startsWith ("Audio/Source");
}

//==============================================================================
// A description of a single sink or source node in the PipeWire graph,
// collected during scanForDevices().
struct PipeWireEndpoint
{
    uint32_t globalId = SPA_ID_INVALID; // id of the node, valid for the server session
    String name;                        // a stable identifier (usually the node.name)
    String displayName;                 // a human readable name for the JUCE device lists
    String objectPath;                  // optional object.path, may match the default metadata
    StringArray channels;               // the names of the audio channels, e.g. "FL", "FR"
};

struct PipeWireScanResult
{
    Array<PipeWireEndpoint> sinks, sources;
    String defaultSinkKey, defaultSourceKey;
};

//==============================================================================
class PipeWireRegistryScanner
{
public:
    // Synchronously queries the PipeWire registry. Returns false when the
    // PipeWire server could not be reached.
    static bool scan (PipeWireScanResult& result)
    {
        if (! loadPipeWireLibrary())
            return false;

        PipeWireRegistryScanner scanner;
        return scanner.scanInternal (result);
    }

    //==============================================================================
    void handleCoreDone (uint32_t id, int seq)
    {
        // The seq of a sync reply always has the 1 << 30 bit set.
        if (id == PW_ID_CORE && (seq & (1 << 30)) != 0)
            connection.quit();
    }

    void handleGlobal (uint32_t id, const char* type, const struct spa_dict* props)
    {
        if (String (type) == PW_TYPE_INTERFACE_Node)
        {
            const auto* mediaClass = getProp (props, PW_KEY_MEDIA_CLASS);

            if (mediaClassIsSink (mediaClass) || mediaClassIsSource (mediaClass))
            {
                NodeRecord node;
                node.id = id;
                node.isSink = mediaClassIsSink (mediaClass);
                node.name = getNodeKey (getProp (props, PW_KEY_NODE_NAME), getProp (props, PW_KEY_OBJECT_PATH));
                node.displayName = String (getProp (props, PW_KEY_NODE_DESCRIPTION));
                node.objectPath = String (getProp (props, PW_KEY_OBJECT_PATH));

                if (node.name.isNotEmpty())
                    nodes.add (node);
            }
        }
        else if (String (type) == PW_TYPE_INTERFACE_Port)
        {
            const auto* nodeIdStr = getProp (props, "node.id");

            if (nodeIdStr != nullptr)
            {
                PortRecord port;
                port.id = id;
                port.nodeId = (uint32_t) atoi (nodeIdStr);
                port.channel = String (getProp (props, PW_KEY_AUDIO_CHANNEL));
                port.isInput = String (getProp (props, PW_KEY_PORT_DIRECTION)) == "in";
                ports.add (port);
            }
        }
        else if (String (type) == PW_TYPE_INTERFACE_Metadata)
        {
            metadataIds.addIfNotAlreadyThere (id);
        }
    }

    void handleMetadataProperty (const char* key, const char* value)
    {
        if (key == nullptr || value == nullptr)
            return;

        if (String (key) == "default.audio.sink")
            result.defaultSinkKey = parseDefaultValue (value);

        if (String (key) == "default.audio.source")
            result.defaultSourceKey = parseDefaultValue (value);
    }

    //==============================================================================
    struct NodeRecord
    {
        uint32_t id = SPA_ID_INVALID;
        bool isSink = false;
        String name, displayName, objectPath;
    };

    struct PortRecord
    {
        uint32_t id = SPA_ID_INVALID;
        uint32_t nodeId = SPA_ID_INVALID;
        String channel;
        bool isInput = false;
    };

private:
    //==============================================================================
    PipeWireRegistryScanner()
    {
        // This struct will be destroyed when the scan is complete, so its
        // listener hooks always outlive the proxies they are attached to.
    }

    ~PipeWireRegistryScanner()
    {
        // The connection (and everything bound to it, including the metadata
        // proxies) is cleaned up when the PipeWireConnection member is
        // destroyed, so we only need to detach our listeners here.
        spa_hook_remove (&coreHook);
        spa_hook_remove (&registryHook);

        for (auto& binding : metadataBindings)
            spa_hook_remove (&binding->objectHook);
    }

    //==============================================================================
    bool scanInternal (PipeWireScanResult& resultToFill)
    {
        result = resultToFill;

        if (! connection.connect())
            return false;

        juce::pw_core_add_listener (connection.core, &coreHook, &getCoreEvents(), this);

        connection.registry = juce::pw_core_get_registry (connection.core, PW_VERSION_REGISTRY, 0);

        if (connection.registry == nullptr)
            return false;

        juce::pw_registry_add_listener (connection.registry, &registryHook, &getRegistryEvents(), this);

        // Wait for the initial burst of globals to arrive.
        juce::pw_core_sync (connection.core, PW_ID_CORE, ++syncCounter);
        connection.runUntilQuit();

        // The "default" metadata contains the names of the default sink and
        // source. We bind to all metadata objects and read their properties to
        // find those names.
        for (auto metadataId : metadataIds)
            bindMetadata (metadataId);

        if (metadataBindings.size() > 0)
        {
            // A final sync ensures that all the property events have arrived
            // before we proceed.
            juce::pw_core_sync (connection.core, PW_ID_CORE, ++syncCounter);
            connection.runUntilQuit();
        }

        assembleResult();
        resultToFill = result;
        return true;
    }

    //==============================================================================
    void bindMetadata (uint32_t id)
    {
        auto* metadata = (struct pw_metadata*) juce::pw_registry_bind (connection.registry, id,
                                                                 PW_TYPE_INTERFACE_Metadata,
                                                                 PW_VERSION_METADATA, 0);

        if (metadata == nullptr)
            return;

        auto binding = std::make_unique<MetadataBinding>();
        binding->metadata = metadata;
        juce::pw_proxy_add_object_listener ((struct pw_proxy*) metadata, &binding->objectHook,
                                            &getMetadataEvents(), this);
        metadataBindings.add (std::move (binding));
    }

    //==============================================================================
    static const struct pw_core_events& getCoreEvents()
    {
        static struct pw_core_events events {};

        if (events.version == 0)
        {
            events.version = PW_VERSION_CORE_EVENTS;
            events.done = [] (void* data, uint32_t id, int seq)
            {
                static_cast<PipeWireRegistryScanner*> (data)->handleCoreDone (id, seq);
            };
        }

        return events;
    }

    static const struct pw_registry_events& getRegistryEvents()
    {
        static struct pw_registry_events events {};

        if (events.version == 0)
        {
            events.version = PW_VERSION_REGISTRY_EVENTS;
            events.global = [] (void* data, uint32_t id, uint32_t, const char* type,
                                uint32_t, const struct spa_dict* props)
            {
                static_cast<PipeWireRegistryScanner*> (data)->handleGlobal (id, type, props);
            };
            events.global_remove = [] (void*, uint32_t) {};
        }

        return events;
    }

    static const struct pw_metadata_events& getMetadataEvents()
    {
        static struct pw_metadata_events events {};

        if (events.version == 0)
        {
            events.version = PW_VERSION_METADATA_EVENTS;
            events.property = [] (void* data, uint32_t, const char* key, const char*, const char* value)
            {
                static_cast<PipeWireRegistryScanner*> (data)->handleMetadataProperty (key, value);
                return 0;
            };
        }

        return events;
    }

    //==============================================================================
    void assembleResult()
    {
        result.sinks.clear();
        result.sources.clear();

        // Put the ports in a deterministic order (their creation order) so the
        // channels of each device always appear in the same order.
        std::sort (ports.begin(), ports.end(),
                   [] (const PortRecord& a, const PortRecord& b) { return a.id < b.id; });

        for (auto& node : nodes)
        {
            // A sink exposes its channels via its input ports and a source via
            // its output ports.
            auto& endpointList = node.isSink ? result.sinks : result.sources;

            PipeWireEndpoint endpoint;
            endpoint.globalId = node.id;
            endpoint.name = node.name;
            endpoint.displayName = node.displayName.isNotEmpty() ? node.displayName : node.name;
            endpoint.objectPath = node.objectPath;

            for (auto& port : ports)
            {
                if (port.nodeId != node.id || port.isInput != node.isSink || port.channel.isEmpty())
                    continue;

                endpoint.channels.add (port.channel);
            }

            endpointList.add (endpoint);
        }
    }

    static String getNodeKey (const char* nodeName, const char* objectPath)
    {
        if (nodeName != nullptr && String (nodeName).isNotEmpty())
            return String (nodeName);

        if (objectPath != nullptr && String (objectPath).isNotEmpty())
            return String (objectPath);

        return {};
    }

    static String parseDefaultValue (const char* value)
    {
        // The default metadata stores values as JSON, e.g. {"name": "..."}.
        if (auto* namePos = strstr (value, "\"name\""))
        {
            if (auto* quote = strchr (namePos, ':'))
            {
                if (auto* start = strchr (quote, '"'))
                {
                    if (auto* end = strchr (start + 1, '"'))
                        return String (start + 1, (int) (end - start - 1));
                }
            }
        }

        return String (value).trim();
    }

    //==============================================================================
    struct MetadataBinding
    {
        struct pw_metadata* metadata = nullptr;
        struct spa_hook objectHook {};
    };

    struct PipeWireConnection
    {
        PipeWireConnection()
        {
            int fakeArgc = 1;
            char fakeArgv[] = "juce";
            char* fakeArgvPtr = fakeArgv;
            juce::pw_init (&fakeArgc, &fakeArgvPtr);
        }

        ~PipeWireConnection()
        {
            if (core != nullptr)        juce::pw_core_disconnect (core);
            if (context != nullptr)     juce::pw_context_destroy (context);
            if (mainLoop != nullptr)    juce::pw_main_loop_destroy (mainLoop);
        }

        bool connect()
        {
            mainLoop = juce::pw_main_loop_new (nullptr);

            if (mainLoop == nullptr)
                return false;

            context = juce::pw_context_new (juce::pw_main_loop_get_loop (mainLoop), nullptr, 0);

            if (context == nullptr)
                return false;

            core = juce::pw_context_connect (context, nullptr, 0);
            return core != nullptr;
        }

        // Runs the main loop until quit() is called.
        void runUntilQuit()
        {
            quitRequested = false;

            while (! quitRequested)
                juce::pw_main_loop_run (mainLoop);
        }

        void quit()
        {
            quitRequested = true;
            juce::pw_main_loop_quit (mainLoop);
        }

        struct pw_main_loop* mainLoop = nullptr;
        struct pw_context* context = nullptr;
        struct pw_core* core = nullptr;
        struct pw_registry* registry = nullptr;
        bool quitRequested = false;

        JUCE_DECLARE_NON_COPYABLE (PipeWireConnection)
    };

    PipeWireConnection connection;

    Array<NodeRecord> nodes;
    Array<PortRecord> ports;
    Array<uint32_t> metadataIds;
    Array<std::unique_ptr<MetadataBinding>> metadataBindings;
    struct spa_hook coreHook {}, registryHook {};

    PipeWireScanResult result;
    int syncCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PipeWireRegistryScanner)
};

//==============================================================================
class PipeWireAudioIODevice final : public AudioIODevice
{
public:
    PipeWireAudioIODevice (const String& typeNameToUse,
                           const String& sinkKeyToUse,
                           const String& sinkDisplayName,
                           const StringArray& sinkChannelNames,
                           const String& sourceKeyToUse,
                           const String& sourceDisplayName,
                           const StringArray& sourceChannelNames)
        : AudioIODevice (sinkDisplayName.isNotEmpty() ? sinkDisplayName : sourceDisplayName,
                         typeNameToUse),
          sinkKey (sinkKeyToUse),
          sourceKey (sourceKeyToUse),
          outputChannelNames (sinkChannelNames),
          inputChannelNames (sourceChannelNames)
    {
        jassert (sinkKey.isNotEmpty() || sourceKey.isNotEmpty());
    }

    ~PipeWireAudioIODevice() override
    {
        close();
    }

    //==============================================================================
    StringArray getOutputChannelNames() override    { return outputChannelNames; }
    StringArray getInputChannelNames() override     { return inputChannelNames; }

    Array<double> getAvailableSampleRates() override
    {
        Array<double> rates;

        for (auto rate : SampleRateHelpers::getCommonSampleRates())
            rates.add (rate);

        return rates;
    }

    Array<int> getAvailableBufferSizes() override
    {
        // PipeWire decides the actual block size (its "quantum") at run-time,
        // so like the JACK backend we advertise a range of sensible sizes and
        // report the real one via getCurrentBufferSizeSamples().
        Array<int> sizes;

        for (int n = 16; n <= 8192; n += n < 64 ? 16 : (n < 512 ? 32 : (n < 2048 ? 64 : 128)))
            sizes.add (n);

        return sizes;
    }

    int getDefaultBufferSize() override                 { return 512; }

    //==============================================================================
    String open (const BigInteger& inputChannels,
                 const BigInteger& outputChannels,
                 double sampleRate,
                 int bufferSizeSamples) override
    {
        close();

        requestedSampleRate = sampleRate > 0 ? sampleRate : 48000.0;
        requestedBufferSize = bufferSizeSamples > 0 ? bufferSizeSamples : getDefaultBufferSize();

        enabledInputChannels  = restrictToKnownChannels (inputChannels,  inputChannelNames.size());
        enabledOutputChannels = restrictToKnownChannels (outputChannels, outputChannelNames.size());

        if (enabledInputChannels.isZero() && enabledOutputChannels.isZero())
        {
            lastError = "No channels were selected for the PipeWire device";
            return lastError;
        }

        lastError.clear();

        if (! loadPipeWireLibrary())
        {
            lastError = "PipeWire is not available on this system";
            return lastError;
        }

        // Resolve the ids of the requested nodes. Node ids can change when the
        // server restarts or devices are unplugged, so we always re-scan before
        // opening the device.
        PipeWireScanResult scanResult;

        if (! PipeWireRegistryScanner::scan (scanResult))
        {
            lastError = "Could not connect to the PipeWire server";
            return lastError;
        }

        if (sinkKey.isNotEmpty())
        {
            sinkNodeId = findNodeId (scanResult.sinks, sinkKey);

            if (sinkNodeId == SPA_ID_INVALID)
            {
                lastError = "The PipeWire output device \"" + sinkKey + "\" is not available";
                return lastError;
            }
        }

        if (sourceKey.isNotEmpty())
        {
            sourceNodeId = findNodeId (scanResult.sources, sourceKey);

            if (sourceNodeId == SPA_ID_INVALID)
            {
                lastError = "The PipeWire input device \"" + sourceKey + "\" is not available";
                return lastError;
            }
        }

        if (! createStreams())
        {
            close();
            return lastError;
        }

        isOpen_ = true;
        isPlaying_ = false;
        return {};
    }

    void close() override
    {
        stop();

        // Destroy the streams while the loop thread is still running: the
        // destruction requires the loop to dispatch the server's replies.
        destroyStreams();

        if (audioThread != nullptr)
        {
            audioThread->stopThread (2000);
            audioThread.reset();
        }

        if (pwCore != nullptr)
        {
            juce::pw_core_disconnect (pwCore);
            pwCore = nullptr;
        }

        if (pwContext != nullptr)
        {
            juce::pw_context_destroy (pwContext);
            pwContext = nullptr;
        }

        if (pwLoop != nullptr)
        {
            juce::pw_loop_destroy (pwLoop);
            pwLoop = nullptr;
        }

        sinkNodeId = SPA_ID_INVALID;
        sourceNodeId = SPA_ID_INVALID;
        enabledInputChannels.clear();
        enabledOutputChannels.clear();
        captureSamples = 0;
        isOpen_ = false;
        isPlaying_ = false;
    }

    bool isOpen() override                              { return isOpen_; }
    bool isPlaying() override                           { return isPlaying_; }
    String getLastError() override                      { return lastError; }

    //==============================================================================
    void start (AudioIODeviceCallback* newCallback) override
    {
        if (! isOpen_)
            newCallback = nullptr;

        if (newCallback != callback)
        {
            if (newCallback != nullptr)
                newCallback->audioDeviceAboutToStart (this);

            AudioIODeviceCallback* const oldCallback = callback;

            {
                const ScopedLock sl (callbackLock);
                callback = newCallback;
            }

            if (oldCallback != nullptr)
                oldCallback->audioDeviceStopped();
        }

        isPlaying_ = (callback != nullptr);

        setActive (isPlaying_);
    }

    void stop() override
    {
        start (nullptr);
    }

    //==============================================================================
    int getCurrentBufferSizeSamples() override          { return currentBufferSize; }
    double getCurrentSampleRate() override              { return currentSampleRate; }
    int getCurrentBitDepth() override                   { return 32; }

    BigInteger getActiveOutputChannels() const override { return enabledOutputChannels; }
    BigInteger getActiveInputChannels()  const override { return enabledInputChannels; }

    int getOutputLatencyInSamples() override
    {
        auto latency = playbackStream != nullptr ? getLatencyInSamples (playbackStream->stream) : 0;

        // Input data is handed to the callback one block after capture, so add
        // an extra block of latency when doing full-duplex I/O.
        if (playbackStream != nullptr && captureStream != nullptr)
            latency += jmax (1, getCurrentBufferSizeSamples());

        return latency;
    }

    int getInputLatencyInSamples() override
    {
        auto latency = captureStream != nullptr ? getLatencyInSamples (captureStream->stream) : 0;

        if (playbackStream != nullptr && captureStream != nullptr)
            latency += jmax (1, getCurrentBufferSizeSamples());

        return latency;
    }

    int getXRunCount() const noexcept override          { return xruns; }

    std::optional<String> getRoutedOutputDeviceName() const override
    {
        if (sinkKey.isNotEmpty())
            return sinkKey;

        return {};
    }

    //==============================================================================
    String sinkKey, sourceKey;

private:
    //==============================================================================
    static BigInteger restrictToKnownChannels (const BigInteger& requested, int maxChannels)
    {
        auto result = requested;

        if (maxChannels > 0)
            for (int i = maxChannels; i <= result.getHighestBit(); ++i)
                result.clearBit (i);

        return result;
    }

    static uint32_t findNodeId (const Array<PipeWireEndpoint>& endpoints, const String& key)
    {
        for (auto& endpoint : endpoints)
        {
            if (endpoint.name == key || (endpoint.objectPath.isNotEmpty() && endpoint.objectPath == key))
                return endpoint.globalId;
        }

        return SPA_ID_INVALID;
    }

    //==============================================================================
    struct StreamData
    {
        PipeWireAudioIODevice* owner = nullptr;
        struct pw_stream* stream = nullptr;
        struct spa_hook listener {};
        bool isCapture = false;
        int numChannels = 0;
        bool isReady = false;
    };

    bool createStreams()
    {
        pwLoop = juce::pw_loop_new (nullptr);

        if (pwLoop == nullptr)
        {
            lastError = "Could not create a PipeWire loop";
            return false;
        }

        pwContext = juce::pw_context_new (pwLoop, nullptr, 0);

        if (pwContext == nullptr)
        {
            lastError = "Could not create a PipeWire context";
            return false;
        }

        pwCore = juce::pw_context_connect (pwContext, nullptr, 0);

        if (pwCore == nullptr)
        {
            lastError = "Could not connect to the PipeWire server";
            return false;
        }

        if (! enabledOutputChannels.isZero())
        {
            playbackStream = std::make_unique<StreamData>();
            playbackStream->owner = this;
            playbackStream->isCapture = false;
            playbackStream->numChannels = enabledOutputChannels.countNumberOfSetBits();
        }

        if (! enabledInputChannels.isZero())
        {
            captureStream = std::make_unique<StreamData>();
            captureStream->owner = this;
            captureStream->isCapture = true;
            captureStream->numChannels = enabledInputChannels.countNumberOfSetBits();
        }

        // Create the streams before the loop thread starts, and connect them
        // afterwards: pw_stream_connect() waits for a reply from the server,
        // which is only dispatched once the loop is being pumped.
        if (playbackStream != nullptr && ! createStream (*playbackStream))
            return false;

        if (captureStream != nullptr && ! createStream (*captureStream))
            return false;

        audioThread = std::make_unique<AudioThread> (*this);
        audioThread->startThread (Thread::Priority::high);

        if (playbackStream != nullptr && ! connectStream (*playbackStream))
            return false;

        if (captureStream != nullptr && ! connectStream (*captureStream))
            return false;

        return waitForStreamsToStart();
    }

    bool createStream (StreamData& data)
    {
        const auto clientName = "JUCE-PipeWire-" + (data.isCapture ? String ("Input") : String ("Output"))
                                  + "-" + String ((int) ++streamIdCounter);

        auto* props = createEmptyPipeWireProperties();

        if (props == nullptr)
        {
            lastError = "Could not create a PipeWire stream";
            return false;
        }

        juce::pw_properties_set (props, PW_KEY_MEDIA_TYPE, "Audio");
        juce::pw_properties_set (props, PW_KEY_MEDIA_CATEGORY, data.isCapture ? "Capture" : "Playback");
        juce::pw_properties_set (props, PW_KEY_MEDIA_ROLE, "Music");
        juce::pw_properties_set (props, PW_KEY_NODE_NAME, clientName.toRawUTF8());
        juce::pw_properties_set (props, PW_KEY_NODE_DESCRIPTION, clientName.toRawUTF8());

        data.stream = juce::pw_stream_new (pwCore, clientName.toRawUTF8(), props);

        if (data.stream == nullptr)
        {
            juce::pw_properties_free (props);
            lastError = "Could not create a PipeWire stream";
            return false;
        }

        juce::pw_stream_add_listener (data.stream, &data.listener, &getStreamEvents(), &data);
        return true;
    }

    bool connectStream (StreamData& data)
    {
        // Build a format pod requesting floating point, non-interleaved audio.
        uint8_t podBuffer[512];
        auto builder = SPA_POD_BUILDER_INIT (podBuffer, sizeof podBuffer);

        struct spa_audio_info_raw info {};
        info.format = SPA_AUDIO_FORMAT_F32P;
        info.channels = (uint32_t) data.numChannels;
        info.rate = (uint32_t) requestedSampleRate;

        const struct spa_pod* params[1];
        params[0] = spa_format_audio_raw_build (&builder, SPA_PARAM_EnumFormat, &info);

        const auto direction = data.isCapture ? PW_DIRECTION_INPUT : PW_DIRECTION_OUTPUT;
        const auto flags = (enum pw_stream_flags) (PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS);
        const auto targetId = data.isCapture ? sourceNodeId : sinkNodeId;
        const auto result = juce::pw_stream_connect (data.stream, direction, targetId, flags, params, 1);

        if (result < 0)
        {
            lastError = "Could not connect to the PipeWire server (error " + String (-result) + ")";
            return false;
        }

        return true;
    }

    //==============================================================================
    static const struct pw_stream_events& getStreamEvents()
    {
        static struct pw_stream_events events {};

        if (events.version == 0)
        {
            events.version = PW_VERSION_STREAM_EVENTS;
            events.state_changed = [] (void* data, enum pw_stream_state /* oldState */,
                                       enum pw_stream_state state, const char* error)
            {
                auto& streamData = *static_cast<StreamData*> (data);
                streamData.owner->handleStreamStateChanged (streamData, state, error);
            };
            events.param_changed = [] (void* data, uint32_t, const struct spa_pod* param)
            {
                auto& streamData = *static_cast<StreamData*> (data);
                streamData.owner->handleStreamParamChanged (param);
            };
            events.process = [] (void* data)
            {
                auto& streamData = *static_cast<StreamData*> (data);
                streamData.owner->handleStreamProcess (streamData);
            };
        }

        return events;
    }

    //==============================================================================
    void handleStreamStateChanged (StreamData& data, enum pw_stream_state state,
                                   const char* error)
    {
        JUCE_PIPEWIRE_LOG ("PipeWire stream changed state to " << pw_stream_state_as_string (state)
                             << (error != nullptr ? (String (": ") + String (error)) : String()));

        if (state == PW_STREAM_STATE_ERROR)
        {
            {
                const ScopedLock sl (callbackLock);
                lastError = error != nullptr ? String (error) : "The PipeWire stream failed";
            }

            streamsStartedEvent.signal();
            return;
        }

        if (state == PW_STREAM_STATE_PAUSED)
        {
            data.isReady = true;

            const bool allStreamsReady = (playbackStream == nullptr || playbackStream->isReady)
                                      && (captureStream  == nullptr || captureStream->isReady);

            if (allStreamsReady)
                streamsStartedEvent.signal();
        }
    }

    void handleStreamParamChanged (const struct spa_pod* param)
    {
        if (param == nullptr)
            return;

        uint32_t mediaType, mediaSubtype;

        if (spa_format_parse (param, &mediaType, &mediaSubtype) < 0)
            return;

        if (mediaType != SPA_MEDIA_TYPE_audio || mediaSubtype != SPA_MEDIA_SUBTYPE_raw)
            return;

        struct spa_audio_info_raw info {};

        if (spa_format_audio_raw_parse (param, &info) < 0)
            return;

        if (info.rate > 0)
            currentSampleRate = (double) info.rate;
    }

    void handleStreamProcess (StreamData& data)
    {
        if (data.isCapture)
            processCapture (data);
        else
            processPlayback (data);
    }

    //==============================================================================
    void processPlayback (StreamData& data)
    {
        auto* buffer = juce::pw_stream_dequeue_buffer (data.stream);

        if (buffer == nullptr || buffer->buffer == nullptr)
        {
            xruns++;
            return;
        }

        auto* spaBuffer = buffer->buffer;
        const int numSamples = getBufferNumSamples (*buffer);
        const int numChannels = (int) spaBuffer->n_datas;

        if (numSamples <= 0 || numChannels <= 0)
        {
            juce::pw_stream_queue_buffer (data.stream, buffer);
            return;
        }

        currentBufferSize = numSamples;

        if (outputPtrCapacity < numChannels)
        {
            outputPtrs.calloc ((size_t) numChannels + 2);
            outputPtrCapacity = numChannels + 2;
        }

        for (int i = 0; i < numChannels; ++i)
            outputPtrs[i] = (float*) spaBuffer->datas[i].data;

        // If we're recording as well as playing, point the input channels at
        // the data that was captured in the most recent capture callback.
        const auto numInputChannels = captureBuffer.getNumChannels();
        float** inputData = nullptr;

        if (numInputChannels > 0)
        {
            if (inputPtrCapacity < numInputChannels)
            {
                inputPtrs.calloc ((size_t) numInputChannels + 2);
                inputPtrCapacity = numInputChannels + 2;
            }

            inputData = inputPtrs.getData();

            if (captureBuffer.getNumSamples() < numSamples)
            {
                // The capture buffer is smaller than the playback block, which
                // shouldn't normally happen. Discard the captured data.
                captureSamples = 0;
                xruns++;
            }
            else if (captureSamples > numSamples)
            {
                // The capture produced more data than this block needs, so
                // just take the most recent samples.
                const auto extra = captureSamples - numSamples;
                captureBuffer.clear (0, extra);
                captureSamples = numSamples;
            }
            else if (captureSamples < numSamples)
            {
                // Pad the input with silence.
                captureBuffer.clear (captureSamples, numSamples - captureSamples);
            }

            for (int i = 0; i < numInputChannels; ++i)
                inputPtrs[i] = const_cast<float*> (captureBuffer.getReadPointer (i, 0));
        }

        {
            const ScopedLock sl (callbackLock);

            if (callback != nullptr)
            {
                callback->audioDeviceIOCallbackWithContext (inputData, numInputChannels,
                                                            outputPtrs.getData(), numChannels,
                                                            numSamples, {});
            }
            else
            {
                for (int i = 0; i < numChannels; ++i)
                    if (outputPtrs[i] != nullptr)
                        zeromem (outputPtrs[i], (size_t) numSamples * sizeof (float));
            }
        }

        for (uint32_t i = 0; i < spaBuffer->n_datas; ++i)
        {
            auto& chunk = *spaBuffer->datas[i].chunk;
            chunk.size = (uint32_t) (numSamples * 4);
            chunk.stride = 4;
        }

        captureSamples = 0;
        juce::pw_stream_queue_buffer (data.stream, buffer);
    }

    void processCapture (StreamData& data)
    {
        auto* buffer = juce::pw_stream_dequeue_buffer (data.stream);

        if (buffer == nullptr || buffer->buffer == nullptr)
        {
            xruns++;
            return;
        }

        auto* spaBuffer = buffer->buffer;
        const int numSamples = getBufferNumSamples (*buffer);
        const int numChannels = (int) spaBuffer->n_datas;

        if (numSamples <= 0 || numChannels <= 0)
        {
            juce::pw_stream_queue_buffer (data.stream, buffer);
            return;
        }

        // If we're only recording, drive the callback directly from here,
        // otherwise store the data for the next playback block.
        if (playbackStream == nullptr)
        {
            currentBufferSize = numSamples;

            if (inputPtrCapacity < numChannels)
            {
                inputPtrs.calloc ((size_t) numChannels + 2);
                inputPtrCapacity = numChannels + 2;
            }

            for (int i = 0; i < numChannels; ++i)
                inputPtrs[i] = (float*) spaBuffer->datas[i].data;

            const ScopedLock sl (callbackLock);

            if (callback != nullptr)
                callback->audioDeviceIOCallbackWithContext (inputPtrs.getData(), numChannels,
                                                            nullptr, 0, numSamples, {});
        }
        else
        {
            captureBuffer.setSize (numChannels, jmax (numSamples, 8192), false, false, true);
            captureBuffer.clear();

            for (int i = 0; i < numChannels; ++i)
                if (spaBuffer->datas[i].data != nullptr)
                    memcpy (captureBuffer.getWritePointer (i, 0),
                            spaBuffer->datas[i].data,
                            (size_t) numSamples * sizeof (float));

            captureSamples = numSamples;
        }

        for (uint32_t i = 0; i < spaBuffer->n_datas; ++i)
        {
            auto& chunk = *spaBuffer->datas[i].chunk;
            chunk.size = (uint32_t) (numSamples * 4);
            chunk.stride = 4;
        }

        juce::pw_stream_queue_buffer (data.stream, buffer);
    }

    //==============================================================================
    static int getBufferNumSamples (const struct pw_buffer& buffer)
    {
        if (buffer.requested > 0)
            return (int) buffer.requested;

        if (buffer.buffer == nullptr || buffer.buffer->n_datas == 0)
            return 0;

        const auto& data = buffer.buffer->datas[0];

        if (data.chunk != nullptr && data.chunk->stride > 0)
            return (int) (data.chunk->size / data.chunk->stride);

        if (data.chunk != nullptr)
            return (int) (data.chunk->size / 4);

        return 0;
    }

    static int getLatencyInSamples (struct pw_stream* stream)
    {
        struct pw_time time {};

        if (juce::pw_stream_get_time (stream, &time) == 0 && time.delay > 0 && time.rate.denom > 0)
            return (int) ((double) time.delay * (double) time.rate.denom / (double) jmax (1u, time.rate.num));

        return 0;
    }

    //==============================================================================
    bool waitForStreamsToStart()
    {
        if (! streamsStartedEvent.wait (10000))
        {
            const ScopedLock sl (callbackLock);
            lastError = "Timed out waiting for the PipeWire server to start the streams";
        }

        const ScopedLock sl (callbackLock);
        return lastError.isEmpty();
    }

    //==============================================================================
    void destroyStreams()
    {
        if (playbackStream != nullptr)
        {
            if (playbackStream->stream != nullptr)
                juce::pw_stream_destroy (playbackStream->stream);

            playbackStream.reset();
        }

        if (captureStream != nullptr)
        {
            if (captureStream->stream != nullptr)
                juce::pw_stream_destroy (captureStream->stream);

            captureStream.reset();
        }
    }

    void setActive (bool shouldBeActive)
    {
        if (playbackStream != nullptr)   juce::pw_stream_set_active (playbackStream->stream, shouldBeActive);
        if (captureStream  != nullptr)   juce::pw_stream_set_active (captureStream->stream,  shouldBeActive);
    }

    //==============================================================================
    class AudioThread final : public Thread
    {
    public:
        explicit AudioThread (PipeWireAudioIODevice& ownerToUse)
            : Thread ("PipeWire Audio"), owner (ownerToUse)
        {
        }

        void run() override
        {
            while (! threadShouldExit())
                juce::pw_loop_iterate (owner.pwLoop, 100);
        }

    private:
        PipeWireAudioIODevice& owner;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioThread)
    };

    //==============================================================================
    StringArray outputChannelNames, inputChannelNames;
    String lastError;
    CriticalSection callbackLock;

    AudioIODeviceCallback* callback = nullptr;

    std::unique_ptr<StreamData> playbackStream, captureStream;
    std::unique_ptr<AudioThread> audioThread;

    struct pw_loop* pwLoop = nullptr;
    struct pw_context* pwContext = nullptr;
    struct pw_core* pwCore = nullptr;

    HeapBlock<float*> outputPtrs, inputPtrs;
    int outputPtrCapacity = 0, inputPtrCapacity = 0;
    AudioBuffer<float> captureBuffer;
    int captureSamples = 0;

    double requestedSampleRate = 48000.0;
    int requestedBufferSize = 512;
    double currentSampleRate = 0.0;
    int currentBufferSize = 0;
    std::atomic<int> xruns { 0 };

    uint32_t sinkNodeId = SPA_ID_INVALID;
    uint32_t sourceNodeId = SPA_ID_INVALID;
    uint32_t streamIdCounter = 0;

    BigInteger enabledInputChannels, enabledOutputChannels;
    bool isOpen_ = false, isPlaying_ = false;
    WaitableEvent streamsStartedEvent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PipeWireAudioIODevice)
};

//==============================================================================
class PipeWireAudioIODeviceType final : public AudioIODeviceType
{
public:
    PipeWireAudioIODeviceType()
        : AudioIODeviceType ("PipeWire")
    {
    }

    //==============================================================================
    void scanForDevices() override
    {
        outputNames.clear();
        outputKeys.clear();
        outputChannels.clear();
        inputNames.clear();
        inputKeys.clear();
        inputChannels.clear();
        defaultOutputKey.clear();
        defaultInputKey.clear();

        PipeWireScanResult result;

        if (! PipeWireRegistryScanner::scan (result))
        {
            hasScanned = true;
            return;
        }

        appendDevices (result.sinks,   result.defaultSinkKey,   outputNames, outputKeys, outputChannels, defaultOutputKey);
        appendDevices (result.sources, result.defaultSourceKey, inputNames,  inputKeys,  inputChannels,  defaultInputKey);

        outputNames.appendNumbersToDuplicates (false, true);
        inputNames.appendNumbersToDuplicates (false, true);

        hasScanned = true;
    }

    StringArray getDeviceNames (bool wantInputNames) const override
    {
        jassert (hasScanned); // need to call scanForDevices() before doing this
        return wantInputNames ? inputNames : outputNames;
    }

    int getDefaultDeviceIndex (bool forInput) const override
    {
        jassert (hasScanned); // need to call scanForDevices() before doing this

        const auto& keys = forInput ? inputKeys : outputKeys;
        const auto& defaultKey = forInput ? defaultInputKey : defaultOutputKey;

        if (defaultKey.isNotEmpty())
        {
            const auto index = keys.indexOf (defaultKey);

            if (index >= 0)
                return index;
        }

        return 0;
    }

    bool hasSeparateInputsAndOutputs() const override     { return true; }

    int getIndexOfDevice (AudioIODevice* device, bool asInput) const override
    {
        jassert (hasScanned); // need to call scanForDevices() before doing this

        if (auto* d = dynamic_cast<PipeWireAudioIODevice*> (device))
            return asInput ? inputKeys.indexOf (d->sourceKey)
                           : outputKeys.indexOf (d->sinkKey);

        return -1;
    }

    AudioIODevice* createDevice (const String& outputDeviceName,
                                 const String& inputDeviceName) override
    {
        jassert (hasScanned); // need to call scanForDevices() before doing this

        const auto inputIndex  = inputNames.indexOf (inputDeviceName);
        const auto outputIndex = outputNames.indexOf (outputDeviceName);

        if (inputIndex < 0 && outputIndex < 0)
            return nullptr;

        const auto hasOutput = outputIndex >= 0;

        return new PipeWireAudioIODevice (getTypeName(),
                                          hasOutput ? outputKeys.getReference (outputIndex) : String(),
                                          hasOutput ? outputDeviceName : String(),
                                          hasOutput ? outputChannels.getReference (outputIndex) : StringArray(),
                                          inputIndex >= 0 ? inputKeys.getReference (inputIndex) : String(),
                                          inputIndex >= 0 ? inputDeviceName : String(),
                                          inputIndex >= 0 ? inputChannels.getReference (inputIndex) : StringArray());
    }

private:
    //==============================================================================
    static void appendDevices (const Array<PipeWireEndpoint>& endpoints,
                               const String& defaultKey,
                               StringArray& names,
                               StringArray& keys,
                               Array<StringArray>& channels,
                               String& defaultKeyOut)
    {
        for (auto& endpoint : endpoints)
        {
            if (endpoint.name.isEmpty())
                continue;

            names.add (endpoint.displayName);
            keys.add (endpoint.name);
            channels.add (endpoint.channels);

            // The default device is identified in the metadata by its node.name
            // or its object.path.
            if (defaultKey.isNotEmpty() && defaultKeyOut.isEmpty()
                 && (endpoint.name == defaultKey || endpoint.objectPath == defaultKey))
                defaultKeyOut = endpoint.name;
        }
    }

    StringArray outputNames, outputKeys, inputNames, inputKeys;
    Array<StringArray> outputChannels, inputChannels;
    String defaultOutputKey, defaultInputKey;
    bool hasScanned = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PipeWireAudioIODeviceType)
};

//==============================================================================
// This is called from AudioIODeviceType::createAudioIODeviceType_PipeWire(),
// which is compiled into the same translation unit.
static inline AudioIODeviceType* createAudioIODeviceType_PipeWire_Native()
{
    if (! loadPipeWireLibrary())
        return nullptr;

    return new PipeWireAudioIODeviceType();
}

} // namespace juce
