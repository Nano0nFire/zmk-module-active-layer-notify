import { useContext, useState } from "react";
import "./App.css";
import { connect as serial_connect } from "@zmkfirmware/zmk-studio-ts-client/transport/serial";
import {
  ZMKConnection,
  ZMKCustomSubsystem,
  ZMKAppContext,
} from "@cormoran/zmk-studio-react-hook";
import { Request, Response } from "./proto/nano0nfire/active_layer_notify/active_layer_notify";

export const SUBSYSTEM_IDENTIFIER = "nano0nfire__active_layer_notify";

function App() {
  return (
    <div className="app">
      <header className="app-header">
        <h1>🔧 Active Layer Notify Module</h1>
        <p>Custom Studio RPC Demo</p>
      </header>

      <ZMKConnection
        renderDisconnected={({ connect, isLoading, error }) => (
          <section className="card">
            <h2>Device Connection</h2>
            {isLoading && <p>⏳ Connecting...</p>}
            {error && (
              <div className="error-message">
                <p>🚨 {error}</p>
              </div>
            )}
            {!isLoading && (
              <button
                className="btn btn-primary"
                onClick={() => connect(serial_connect)}
              >
                🔌 Connect Serial
              </button>
            )}
          </section>
        )}
        renderConnected={({ disconnect, deviceName }) => (
          <>
            <section className="card">
              <h2>Device Connection</h2>
              <div className="device-info">
                <h3>✅ Connected to: {deviceName}</h3>
              </div>
              <button className="btn btn-secondary" onClick={disconnect}>
                Disconnect
              </button>
            </section>

            <RPCTestSection />
          </>
        )}
      />

      <footer className="app-footer">
        <p>
          <strong>Active Layer Notify Module</strong> - Customize this for your ZMK module
        </p>
      </footer>
    </div>
  );
}

export function RPCTestSection() {
  const zmkApp = useContext(ZMKAppContext);
  const [response, setResponse] = useState<string | null>(null);
  const [isLoading, setIsLoading] = useState(false);

  if (!zmkApp) return null;

  const subsystem = zmkApp.findSubsystem(SUBSYSTEM_IDENTIFIER);

  const loadActiveLayer = async () => {
    if (!zmkApp.state.connection || !subsystem) return;

    setIsLoading(true);
    setResponse(null);

    try {
      const service = new ZMKCustomSubsystem(
        zmkApp.state.connection,
        subsystem.index
      );

      const request = Request.create({
        getActiveLayer: {},
      });

      const payload = Request.encode(request).finish();
      const responsePayload = await service.callRPC(payload);

      if (responsePayload) {
        const resp = Response.decode(responsePayload);
        console.log("Decoded response:", resp);

        if (resp.getActiveLayer?.layer) {
          const layer = resp.getActiveLayer.layer;
          const layerName = layer.name || `Layer ${layer.index}`;
          setResponse(`${layerName} (index=${layer.index}, id=${layer.id})`);
        } else if (resp.error) {
          setResponse(`Error: ${resp.error.message}`);
        }
      }
    } catch (error) {
      console.error("RPC call failed:", error);
      setResponse(
        `Failed: ${error instanceof Error ? error.message : "Unknown error"}`
      );
    } finally {
      setIsLoading(false);
    }
  };

  if (!subsystem) {
    return (
      <section className="card">
        <div className="warning-message">
          <p>
            ⚠️ Subsystem "{SUBSYSTEM_IDENTIFIER}" not found. Make sure your
            firmware includes the active_layer_notify module.
          </p>
        </div>
      </section>
    );
  }

  return (
    <section className="card">
      <h2>RPC Test</h2>
      <p>Query the firmware for the currently active layer:</p>

      <button
        className="btn btn-primary"
        disabled={isLoading}
        onClick={loadActiveLayer}
      >
        {isLoading ? "⏳ Loading..." : "📤 Get Active Layer"}
      </button>

      {response && (
        <div className="response-box">
          <h3>Response from Firmware:</h3>
          <pre>{response}</pre>
        </div>
      )}
    </section>
  );
}

export default App;
