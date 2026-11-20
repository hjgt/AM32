// One explicit A -> P1 bench migration. No throttle or EEPROM-setting writes.
// preflight is read-only; flash requires fresh operator stop authorization.
const fs = require('node:fs');
const path = require('node:path');
const net = require('node:net');
const crypto = require('node:crypto');
const assert = require('node:assert/strict');
const {spawnSync} = require('node:child_process');
const {parseHex, symbols, header, decodeBench} = require('./fd6288_startup_capture.cjs');
const root = path.resolve(__dirname, '../..');
const file = p => path.join(root, p);
const sha = b => crypto.createHash('sha256').update(b).digest('hex');
const oldElf = file('obj-startup-trace-A/AM32_CUSTOM_FD6288_G431_2.20.elf');
const newElf = file('obj-startup-power-P1/AM32_CUSTOM_FD6288_G431_2.20.elf');
const newHex = file('test-firmware/fd6288-startup-power-20260921/AM32_CUSTOM_FD6288_G431_P1_BENCH_300ms.hex');
const oldBytes = fs.readFileSync(oldElf.replace(/elf$/, 'hex'));
const newBytes = fs.readFileSync(newHex);
assert.equal(sha(oldBytes), '24ff2551ca5908ce1afc421003b9a8e4d0725958540749ce3239f9449c7a82ee');
assert.equal(sha(newBytes), '74619c4e6f7e149e068973c8c4f32f167e8f51fe4052cdb32c5b1224eefa66d2');
const oldImage = parseHex(oldBytes.toString()), newImage = parseHex(newBytes.toString());
function paired(elf, image) {
    const r = spawnSync('arm-none-eabi-objcopy', ['-O', 'ihex', elf, '/dev/stdout'], {encoding: 'utf8'});
    assert.equal(r.status, 0, r.stderr);
    assert.deepEqual(parseHex(r.stdout), image, 'Wrong ELF/HEX pair');
    return symbols(elf);
}
const oldSyms = paired(oldElf, oldImage), newSyms = paired(newElf, newImage);
async function connect() {
    const s = net.createConnection({host: '127.0.0.1', port: 6666});
    s.setTimeout(15000, () => s.destroy(new Error('OpenOCD timeout')));
    await new Promise((resolve, reject) => {s.once('connect', resolve); s.once('error', reject);});
    let pending, buffer = '';
    s.on('data', bytes => {
        buffer += bytes.toString();
        const i = buffer.indexOf('\x1a');
        if (i >= 0 && pending) {
            const p = pending; pending = null;
            const reply = buffer.slice(0, i); buffer = buffer.slice(i + 1); p.resolve(reply);
        }
    });
    const failed = e => {if (pending) {pending.reject(e); pending = null;}};
    s.on('error', failed); s.on('close', () => failed(new Error('OpenOCD closed')));
    return {close: () => s.destroy(), rpc: command => new Promise((resolve, reject) => {
        assert(!pending); pending = {resolve, reject};
        s.write(`set _p1_result {}; if {[catch {${command}} _p1_result]} {format "ERR:%s" $_p1_result} else {format "OK:%s" $_p1_result}\x1a`);
    }).then(reply => {assert(reply.startsWith('OK:'), reply); return reply.slice(3);})};
}
function imageMatches(image, flash) {
    assert.equal(flash.length, 65536);
    for (const [a, b] of image) assert.equal(flash[a - 0x08000000], b, `Firmware mismatch at 0x${a.toString(16)}`);
}
async function main() {
    const [mode, folder, confirmation] = process.argv.slice(2);
    assert(['identify', 'preflight', 'flash', 'verify'].includes(mode), 'Choose identify, preflight, flash, or verify');
    assert.equal(confirmation, '--operator-stopped');
    assert(folder && /^[\w/.-]+$/.test(folder), 'Explicit backup path required');
    const directory = path.resolve(folder);
    assert(directory.startsWith(file('hardware-backups/') + 'startup-P1-flash-'));
    assert(fs.statSync(directory).isDirectory());
    const save = (name, data) => fs.writeFileSync(path.join(directory, name), data, {flag: 'wx'});
    const operations = [];
    const log = (stage, value) => {const entry = {at: new Date().toISOString(), stage, value}; operations.push(entry); console.log(JSON.stringify(entry));};
    const c = await connect();
    async function memory(a, count) {
        const b = Buffer.alloc(count), stride = a % 4 || count % 4 ? 1 : 4;
        for (let i = 0; i < count; i += 1024) {
            const n = Math.min(1024, count - i);
            const r = await c.rpc(`read_memory 0x${(a + i).toString(16)} ${stride * 8} ${n / stride}`);
            const v = r.trim().split(/\s+/).map(Number);
            assert(v.length === n / stride && v.every(x => Number.isInteger(x) && x >= 0 && x <= (stride === 4 ? 0xffffffff : 255)), 'Bad memory reply');
            v.forEach((x, j) => b.writeUIntLE(x, i + stride * j, stride));
        }
        return b;
    }
    const u32 = async a => (await memory(a, 4)).readUInt32LE();
    async function scalar(syms, n) {
        const s = syms[n];
        assert(s && s.size <= 4 && s.address >= 0x20000000 && s.address + s.size <= 0x20008000, `Bad symbol ${n}`);
        const b = await memory(s.address, s.size);
        return n === 'actual_current' ? b.readIntLE(0, s.size) : b.readUIntLE(0, s.size);
    }
    async function stopped(syms) {
        assert.equal((await c.rpc('[target current] curstate')).trim(), 'running');
        assert.equal(await u32(0xe000ed08), 0x08001000, 'Not in main firmware');
        const r = {};
        for (const n of ['input', 'newinput', 'adjusted_input', 'running', 'heli_coast_active', 'duty_cycle']) {
            r[n] = await scalar(syms, n); assert.equal(r[n], 0, `Not stopped: ${n}`);
        }
        r.gpioa_idr = await u32(0x48000010); r.gpiof_idr = await u32(0x48001410);
        assert.equal(r.gpioa_idr & 0x1f00, 0); assert.equal(r.gpiof_idr & 1, 0);
        r.ccr = [];
        for (let i = 0; i < 4; i++) {r.ccr[i] = await u32(0x40012c34 + 4 * i); assert.equal(r.ccr[i], 0);}
        return r;
    }
    async function compatible(syms, flash) {
        const r = {};
        for (const n of ['armed', 'minimum_duty_cycle', 'min_startup_duty', 'startup_max_duty_cycle', 'SystemCoreClock', 'use_speed_control_loop', 'stepper_sine', 'battery_voltage', 'actual_current', 'stuck_rotor_inhibit', 'stuck_rotor_trip_count', 'drive_safety_limit_inhibit', 'heli_coast_restart_inhibit']) r[n] = await scalar(syms, n);
        for (const [n, value] of Object.entries({armed: 1, minimum_duty_cycle: 210, min_startup_duty: 360, startup_max_duty_cycle: 610, SystemCoreClock: 160000000, use_speed_control_loop: 0, stepper_sine: 0, stuck_rotor_inhibit: 0, drive_safety_limit_inhibit: 0, heli_coast_restart_inhibit: 0})) assert.equal(r[n], value, `Incompatible runtime ${n}`);
        assert(r.battery_voltage >= 600 && r.battery_voltage <= 900, 'Check 2S power supply');
        const s = syms.eepromBuffer, settings = await memory(s.address, s.size);
        assert.deepEqual(settings, flash.subarray(0xf800, 0xf800 + s.size), 'RAM/Flash settings differ');
        for (const [offset, value] of [[6, 1], [18, 0], [19, 0], [20, 1], [21, 0], [22, 0], [24, 24], [25, 150], [38, 0]]) assert.equal(settings[offset], value, `Incompatible EEPROM byte ${offset}`);
        r.direction = settings[17]; r.auto_advance = settings[47]; r.settings_sha256 = sha(flash.subarray(0xf800));
        return r;
    }
    try {
        if (mode === 'identify') {
            assert.equal((await c.rpc('[target current] curstate')).trim(), 'running');
            const flash = await memory(0x08000000, 65536);
            save('flash64k-discovery.bin', flash);
            const matches = [];
            for (const dir of fs.readdirSync(root).filter(n => n === 'obj' || n.startsWith('obj-'))) {
                const hex = file(`${dir}/AM32_CUSTOM_FD6288_G431_2.20.hex`);
                if (!fs.existsSync(hex)) continue;
                try {imageMatches(parseHex(fs.readFileSync(hex, 'utf8')), flash); matches.push(hex);}
                catch { /* read-only comparison to all locally available known candidates */ }
            }
            log('identity', {flash_sha256: sha(flash), matches});
        } else if (mode === 'preflight') {
            // Hardware-independent stopped bridge checks precede flash identity;
            // SRAM interpretation is trusted only after every defined byte matches A.
            assert.equal((await c.rpc('[target current] curstate')).trim(), 'running');
            assert.equal((await u32(0x48000010)) & 0x1f00, 0);
            assert.equal((await u32(0x48001410)) & 1, 0);
            const flash = await memory(0x08000000, 65536);
            imageMatches(oldImage, flash);
            if (fs.existsSync(path.join(directory, 'flash64k-discovery.bin')))
                assert.deepEqual(flash, fs.readFileSync(path.join(directory, 'flash64k-discovery.bin')), 'Full flash read not repeatable');
            const state = await stopped(oldSyms), config = await compatible(oldSyms, flash);
            save('flash64k-preflight.bin', flash);
            save('ram32k-preflight.bin', await memory(0x20000000, 32768));
            await stopped(oldSyms);
            log('preflight_ok', {directory, flash_sha256: sha(flash), state, config});
        } else if (mode === 'flash') {
            assert(!fs.existsSync(path.join(directory, 'flash64k-before.bin')), 'Flash attempt already started; inspect, do not retry blindly');
            const preflight = fs.readFileSync(path.join(directory, 'flash64k-preflight.bin'));
            await stopped(oldSyms);
            const before = await memory(0x08000000, 65536);
            assert.deepEqual(before, preflight, 'Flash changed since preflight');
            imageMatches(oldImage, before);
            await compatible(oldSyms, before);
            save('flash64k-before.bin', before);
            log('before_reset', await stopped(oldSyms));
            log('reset_init', await c.rpc('reset init'));
            log('write_main', await c.rpc(`flash write_image erase {${newHex}}`));
            log('verify_main', await c.rpc(`verify_image {${newHex}}`));
            const after = await memory(0x08000000, 65536);
            save('flash64k-after-halted.bin', after);
            imageMatches(newImage, after);
            assert.deepEqual(after.subarray(0, 0x1000), before.subarray(0, 0x1000), 'BL changed');
            assert.deepEqual(after.subarray(0xf800), before.subarray(0xf800), 'Settings changed');
            log('all_regions_verified', {main_bytes: newImage.size, bootloader_unchanged: true, settings_unchanged: true, flash_sha256: sha(after)});
            log('reset_run', await c.rpc('reset run'));
        } else {
            const before = fs.readFileSync(path.join(directory, 'flash64k-before.bin'));
            const flash = await memory(0x08000000, 65536);
            imageMatches(newImage, flash);
            assert.deepEqual(flash.subarray(0, 0x1000), before.subarray(0, 0x1000));
            assert.deepEqual(flash.subarray(0xf800), before.subarray(0xf800));
            const state = await stopped(newSyms), config = await compatible(newSyms, flash);
            const trace = header(await memory(newSyms.fd_startup_trace.address, 104));
            assert(trace.variant === 3 && trace.state === 1 && trace.sample_sequence === 0 && trace.event_sequence === 0, 'P1 not boot-armed/unused');
            const bench = decodeBench(await memory(newSyms.fd_power_bench.address, 552));
            assert(bench.header.extra === 40 && bench.header.trials === 0 && bench.header.stops === 0 && bench.header.inhibit === 0);
            const policy = await memory(newSyms.fd_power_policy.address, 16);
            assert.equal(policy[10], 1, 'Bench has not received explicit zero');
            const clock = {ctrl: await u32(0xe0001000), first: await u32(0xe0001004)};
            clock.second = await u32(0xe0001004);
            assert((clock.ctrl & 1) && ((clock.second - clock.first) >>> 0) > 0, 'DWT not ticking');
            const regs = {arr: await u32(0x40012c2c), bdtr: await u32(0x40012c44)};
            assert.equal(regs.arr, 6665); assert.equal(regs.bdtr & 255, 0xce);
            save('ram32k-after-running.bin', await memory(0x20000000, 32768));
            await stopped(newSyms);
            log('runtime_verified', {state, config, trace, bench, clock, regs, settings_unchanged: true, bootloader_unchanged: true, note: 'P1 flashed; stopped-state checks only. No motor trial or shutdown-timing validation.'});
        }
    } catch (error) {log('error', {message: error.message}); throw error;}
    finally {c.close(); save(`${mode}-operation-${Date.now()}.json`, JSON.stringify(operations, null, 2) + '\n');}
}
main().catch(e => {console.error(e.stack); process.exitCode = 1;});
