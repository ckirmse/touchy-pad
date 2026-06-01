//! `touchy-opendeck` — OpenDeck device plugin for Touchy-Pad hardware.
//!
//! Run by OpenDeck as a child process with WebSocket connection args
//! on `argv`. See `docs/opendeck-device-plugin.md` for the protocol
//! and `README.md` for an architecture overview.

mod layout;
mod plugin;

use openaction::OpenActionResult;
use plugin::TouchyPlugin;

#[tokio::main(flavor = "multi_thread")]
async fn main() -> OpenActionResult<()> {
	// Log to stdout, exactly like the reference openaction plugin
	// (tools/reference/opendeck-akp153). The OpenAction link is a
	// *separate* TCP WebSocket (`ws://localhost:<port>`), not the
	// process stdio — so writing to stdout cannot corrupt it. OpenDeck
	// redirects the plugin's stdout+stderr into
	// `<opendeck-log-dir>/plugins/<plugin-uuid>.log`; that file is
	// where these lines land. See README.md → "Logs".
	//
	// Default level is Info. Bump it for a debugging run with
	// `TOUCHY_LOG=debug` (or `RUST_LOG=debug`) in the environment.
	let level = std::env::var("TOUCHY_LOG")
		.or_else(|_| std::env::var("RUST_LOG"))
		.ok()
		.and_then(|s| s.trim().parse::<log::LevelFilter>().ok())
		.unwrap_or(log::LevelFilter::Info);
	let _ = simplelog::TermLogger::init(level, simplelog::Config::default(), simplelog::TerminalMode::Stdout, simplelog::ColorChoice::Never);

	let args: Vec<String> = std::env::args().collect();
	log::info!("touchy-opendeck starting (log level {level}); argv = {args:?}");

	// `set_global_event_handler` needs a `&'static dyn GlobalEventHandler`.
	// The plugin lives for the whole process lifetime, so leak it.
	let plugin: &'static TouchyPlugin = Box::leak(Box::new(TouchyPlugin::new()));
	openaction::global_events::set_global_event_handler(plugin);

	log::info!("connecting to OpenDeck WebSocket and entering event loop");
	let result = openaction::run(args).await;
	match &result {
		Ok(()) => log::info!("openaction::run returned cleanly — shutting down"),
		Err(e) => log::error!("openaction::run exited with error: {e}"),
	}
	result
}
