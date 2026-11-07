# Firmware upload approval

Before every firmware upload attempt, including commands such as
`pio run -e esp32dev -t upload` and device tests that upload firmware, ask the
user for explicit approval and wait for it. A general request to upload or a
previous approval is not approval for another attempt. Do not upload if
approval is declined or unavailable. Use the
[`upload-firmware`](skills/upload-firmware/SKILL.md) skill for firmware uploads.
