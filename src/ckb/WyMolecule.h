#pragma once
/*
 * WyMolecule.h — CKB transaction builder for ESP32
 *
 * Wraps the Molecule-generated blockchain.h (nervosnetwork/ckb-c-stdlib) to
 * provide a simple C++ API for constructing CKB transactions on-device.
 *
 * Typical flow:
 *   1. Build a Script  (lock / type)
 *   2. Build OutPoint  (previous cell reference)
 *   3. Build CellInput (outpoint + since)
 *   4. Build CellOutput (capacity + lock script)
 *   5. Build RawTransaction (assemble inputs/outputs)
 *   6. Hash it with WyMolecule::txSigningHash() → feed to WyAuth::sign()
 *   7. Build witness (WitnessArgs with 65-byte signature in lock field)
 *   8. Build final Transaction and serialise → POST to CKB RPC
 *
 * Memory model:
 *   - mol_builder_t uses heap (malloc/free internally via molecule_builder.h)
 *   - Intermediate mol_seg_t blobs freed immediately after use
 *   - Final serialised bytes owned by caller — free() when done
 *
 * Attribution: wraps nervosnetwork/ckb-c-stdlib (MIT)
 */

#include <Arduino.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "molecule_reader.h"
#include "molecule_builder.h"
#include "blockchain.h"
#include "../auth/WyAuth.h"

/* ── Error codes ──────────────────────────────────────────────────────────── */
#define WYMOL_OK          0
#define WYMOL_ERR_PARAM   1
#define WYMOL_ERR_BUILD   2
#define WYMOL_ERR_HASH    3
#define WYMOL_ERR_OOM     4

/* ── Inline helpers ───────────────────────────────────────────────────────── */
static inline void _le64(uint8_t out[8], uint64_t v) {
    for (int i = 0; i < 8; i++) out[i] = (v >> (i*8)) & 0xff;
}
static inline void _le32(uint8_t out[4], uint32_t v) {
    out[0]=v&0xff; out[1]=(v>>8)&0xff; out[2]=(v>>16)&0xff; out[3]=(v>>24)&0xff;
}

/* ── Build a Bytes (fixvec) from raw buffer ───────────────────────────────── */
static inline mol_seg_res_t _buildBytes(const uint8_t *data, size_t len) {
    mol_builder_t b;
    MolBuilder_Bytes_init(&b);
    for (size_t i = 0; i < len; i++) MolBuilder_Bytes_push(&b, data[i]);
    return MolBuilder_Bytes_build(b);
}

/* ── Build an empty BytesOpt ──────────────────────────────────────────────── */
static inline mol_seg_res_t _buildBytesOptEmpty() {
    mol_builder_t b;
    MolBuilder_BytesOpt_init(&b);
    return MolBuilder_BytesOpt_build(b);
}

/* ─────────────────────────────────────────────────────────────────────────── */

/*
 * WyScript — builds a CKB Script (lock or type)
 * hash_type: 0x00=data, 0x01=type, 0x02=data1, 0x04=data2
 */
struct WyScript {
    uint8_t *data;
    size_t   len;
    bool     built;

    WyScript() : data(nullptr), len(0), built(false) {}
    ~WyScript() { freeData(); }

    int build(const uint8_t code_hash[32], uint8_t hash_type,
              const uint8_t *args, size_t args_len) {
        if (built) freeData();

        mol_seg_res_t args_res = _buildBytes(args, args_len);
        if (args_res.errno != MOL_OK) return WYMOL_ERR_BUILD;

        mol_builder_t b;
        MolBuilder_Script_init(&b);
        MolBuilder_Script_set_code_hash(&b, code_hash, 32);
        MolBuilder_Script_set_hash_type(&b, &hash_type);
        MolBuilder_Script_set_args(&b, args_res.seg.ptr, args_res.seg.len);
        free(args_res.seg.ptr);

        mol_seg_res_t res = mol_table_builder_finalize(b);
        if (res.errno != MOL_OK) return WYMOL_ERR_BUILD;

        data = res.seg.ptr;
        len  = res.seg.len;
        built = true;
        return WYMOL_OK;
    }

    void freeData() {
        if (data) { free(data); data = nullptr; len = 0; built = false; }
    }
};

/*
 * WyOutPoint — tx_hash(32) + index(4 LE) = 36 bytes fixed
 */
struct WyOutPoint {
    uint8_t data[36];
    bool    built;

    WyOutPoint() : built(false) { memset(data, 0, 36); }

    int build(const uint8_t tx_hash[32], uint32_t index) {
        mol_builder_t b;
        MolBuilder_OutPoint_init(&b);
        MolBuilder_OutPoint_set_tx_hash(&b, tx_hash);
        uint8_t idx[4]; _le32(idx, index);
        MolBuilder_OutPoint_set_index(&b, idx);

        mol_seg_res_t res = MolBuilder_OutPoint_build(b);
        if (res.errno != MOL_OK) return WYMOL_ERR_BUILD;

        memcpy(data, res.seg.ptr, 36);
        free(res.seg.ptr);
        built = true;
        return WYMOL_OK;
    }
};

/*
 * WyCellInput — since(8 LE) + previous_output(36) = 44 bytes fixed
 */
struct WyCellInput {
    uint8_t data[44];
    bool    built;

    WyCellInput() : built(false) { memset(data, 0, 44); }

    int build(const WyOutPoint &outpoint, uint64_t since = 0) {
        if (!outpoint.built) return WYMOL_ERR_PARAM;

        mol_builder_t b;
        MolBuilder_CellInput_init(&b);
        uint8_t s[8]; _le64(s, since);
        MolBuilder_CellInput_set_since(&b, s);
        MolBuilder_CellInput_set_previous_output(&b, outpoint.data);

        mol_seg_res_t res = MolBuilder_CellInput_build(b);
        if (res.errno != MOL_OK) return WYMOL_ERR_BUILD;

        memcpy(data, res.seg.ptr, 44);
        free(res.seg.ptr);
        built = true;
        return WYMOL_OK;
    }
};

/*
 * WyCellOutput — capacity(8 LE) + lock(Script) + type(ScriptOpt)
 */
struct WyCellOutput {
    uint8_t *data;
    size_t   len;
    bool     built;

    WyCellOutput() : data(nullptr), len(0), built(false) {}
    ~WyCellOutput() { freeData(); }

    int build(uint64_t capacity_shannons, const WyScript &lock,
              const WyScript *type_script = nullptr) {
        if (built) freeData();
        if (!lock.built) return WYMOL_ERR_PARAM;

        uint8_t cap[8]; _le64(cap, capacity_shannons);

        mol_builder_t type_b;
        MolBuilder_ScriptOpt_init(&type_b);
        if (type_script && type_script->built)
            MolBuilder_ScriptOpt_set(&type_b, type_script->data, type_script->len);
        mol_seg_res_t type_res = MolBuilder_ScriptOpt_build(type_b);

        mol_builder_t b;
        MolBuilder_CellOutput_init(&b);
        MolBuilder_CellOutput_set_capacity(&b, cap, 8);
        MolBuilder_CellOutput_set_lock(&b, lock.data, lock.len);
        MolBuilder_CellOutput_set_type_(&b, type_res.seg.ptr, type_res.seg.len);
        free(type_res.seg.ptr);

        mol_seg_res_t res = mol_table_builder_finalize(b);
        if (res.errno != MOL_OK) return WYMOL_ERR_BUILD;

        data = res.seg.ptr;
        len  = res.seg.len;
        built = true;
        return WYMOL_OK;
    }

    void freeData() {
        if (data) { free(data); data = nullptr; len = 0; built = false; }
    }
};

/*
 * WyTransaction — assembles and serialises a complete CKB transaction.
 *
 * Usage:
 *   WyTransaction tx;
 *   tx.addCellDep(dep_outpoint, 1);        // 0=code, 1=dep_group
 *   tx.addInput(cell_input);
 *   tx.addOutput(cell_output);
 *   uint8_t signing_hash[32];
 *   tx.signingHash(signing_hash);          // → WyAuth::sign()
 *   uint8_t sig[65];
 *   auth.sign(signing_hash, sig);
 *   tx.setWitnessSignature(sig);           // wraps in WitnessArgs
 *   size_t tx_len;
 *   uint8_t *raw = tx.build(&tx_len);     // heap; caller frees
 */
class WyTransaction {
public:
    static const int MAX_INPUTS   = 8;
    static const int MAX_OUTPUTS  = 8;
    static const int MAX_DEPS     = 4;
    static const int MAX_WITNESSES = 8;

    WyTransaction() : _n_inputs(0), _n_outputs(0), _n_deps(0), _n_witnesses(0) {
        memset(_input_data, 0, sizeof(_input_data));
        memset(_dep_data,   0, sizeof(_dep_data));
        memset(_output_data, 0, sizeof(_output_data));
        memset(_odata_data,  0, sizeof(_odata_data));
        memset(_output_len,  0, sizeof(_output_len));
        memset(_odata_len,   0, sizeof(_odata_len));
        memset(_witness_data,0, sizeof(_witness_data));
        memset(_witness_len, 0, sizeof(_witness_len));
    }

    ~WyTransaction() {
        for (int i = 0; i < _n_outputs;  i++) { free(_output_data[i]); free(_odata_data[i]); }
        for (int i = 0; i < _n_witnesses;i++) { free(_witness_data[i]); }
    }

    int addCellDep(const WyOutPoint &dep, uint8_t dep_type = 0) {
        if (_n_deps >= MAX_DEPS || !dep.built) return WYMOL_ERR_PARAM;
        memcpy(_dep_data[_n_deps], dep.data, 36);
        _dep_data[_n_deps][36] = dep_type;
        _n_deps++;
        return WYMOL_OK;
    }

    int addInput(const WyCellInput &inp) {
        if (_n_inputs >= MAX_INPUTS || !inp.built) return WYMOL_ERR_PARAM;
        memcpy(_input_data[_n_inputs++], inp.data, 44);
        return WYMOL_OK;
    }

    int addOutput(const WyCellOutput &out,
                  const uint8_t *odata = nullptr, size_t odata_len = 0) {
        if (_n_outputs >= MAX_OUTPUTS || !out.built) return WYMOL_ERR_PARAM;
        int i = _n_outputs;
        _output_data[i] = (uint8_t *)malloc(out.len);
        if (!_output_data[i]) return WYMOL_ERR_OOM;
        memcpy(_output_data[i], out.data, out.len);
        _output_len[i] = out.len;
        _odata_data[i] = (uint8_t *)malloc(odata_len > 0 ? odata_len : 1);
        if (!_odata_data[i]) return WYMOL_ERR_OOM;
        if (odata && odata_len) memcpy(_odata_data[i], odata, odata_len);
        _odata_len[i] = odata_len;
        _n_outputs++;
        return WYMOL_OK;
    }

    /*
     * signingHash() — compute the tx signing hash per CKB spec:
     *   H = Blake2b(raw_tx_hash || witness_len(8 LE) || witness_args_with_zero_lock)
     */
    int signingHash(uint8_t hash_out[32]) {
        uint8_t *raw_tx = nullptr; size_t raw_len = 0;
        int r = _buildRawTx(&raw_tx, &raw_len);
        if (r != WYMOL_OK) return r;

        uint8_t raw_hash[32];
        WyAuth::hashCkb(raw_tx, raw_len, raw_hash);
        free(raw_tx);

        /* Placeholder witness: WitnessArgs with 65 zero bytes in lock field */
        uint8_t placeholder[65] = {0};
        uint8_t *wa = nullptr; size_t wa_len = 0;
        _buildWitnessArgs(placeholder, 65, &wa, &wa_len);

        /* H = Blake2b(raw_hash(32) || wa_len(8 LE) || wa) */
        size_t   buf_len = 32 + 8 + wa_len;
        uint8_t *buf = (uint8_t *)malloc(buf_len);
        if (!buf) { free(wa); return WYMOL_ERR_OOM; }
        memcpy(buf, raw_hash, 32);
        _le64(buf + 32, (uint64_t)wa_len);
        memcpy(buf + 40, wa, wa_len);
        free(wa);

        WyAuth::hashCkb(buf, buf_len, hash_out);
        free(buf);
        return WYMOL_OK;
    }

    /*
     * setWitnessSignature() — wrap the 65-byte sig in WitnessArgs and add as witness[0].
     * Call after signingHash() + WyAuth::sign().
     */
    int setWitnessSignature(const uint8_t sig[65]) {
        uint8_t *wa = nullptr; size_t wa_len = 0;
        _buildWitnessArgs(sig, 65, &wa, &wa_len);
        if (!wa) return WYMOL_ERR_BUILD;

        /* Wrap WitnessArgs in a Bytes (the witness field is a Bytes) */
        mol_seg_res_t bytes_res = _buildBytes(wa, wa_len);
        free(wa);
        if (bytes_res.errno != MOL_OK) return WYMOL_ERR_BUILD;

        /* Replace or set witness[0] */
        if (_n_witnesses == 0) {
            _witness_data[0] = bytes_res.seg.ptr;
            _witness_len[0]  = bytes_res.seg.len;
            _n_witnesses = 1;
        } else {
            free(_witness_data[0]);
            _witness_data[0] = bytes_res.seg.ptr;
            _witness_len[0]  = bytes_res.seg.len;
        }
        return WYMOL_OK;
    }

    /*
     * build() — serialise the full Transaction.
     * Returns heap-allocated bytes (caller must free), writes len to out_len.
     */
    uint8_t *build(size_t *out_len) {
        uint8_t *raw_tx = nullptr; size_t raw_len = 0;
        if (_buildRawTx(&raw_tx, &raw_len) != WYMOL_OK) return nullptr;

        mol_builder_t wvec;
        MolBuilder_BytesVec_init(&wvec);
        for (int i = 0; i < _n_witnesses; i++)
            MolBuilder_BytesVec_push(&wvec, _witness_data[i], _witness_len[i]);
        mol_seg_res_t wvec_res = MolBuilder_BytesVec_build(wvec);

        mol_builder_t tx;
        MolBuilder_Transaction_init(&tx);
        MolBuilder_Transaction_set_raw(&tx, raw_tx, raw_len);
        MolBuilder_Transaction_set_witnesses(&tx, wvec_res.seg.ptr, wvec_res.seg.len);
        free(wvec_res.seg.ptr);
        free(raw_tx);

        mol_seg_res_t res = mol_table_builder_finalize(tx);
        if (res.errno != MOL_OK) return nullptr;
        *out_len = res.seg.len;
        return res.seg.ptr;
    }

private:
    int     _n_inputs;
    uint8_t _input_data[MAX_INPUTS][44];

    int     _n_outputs;
    uint8_t *_output_data[MAX_OUTPUTS];
    size_t   _output_len[MAX_OUTPUTS];
    uint8_t *_odata_data[MAX_OUTPUTS];
    size_t   _odata_len[MAX_OUTPUTS];

    int     _n_deps;
    uint8_t _dep_data[MAX_DEPS][37];

    int     _n_witnesses;
    uint8_t *_witness_data[MAX_WITNESSES];
    size_t   _witness_len[MAX_WITNESSES];

    int _buildRawTx(uint8_t **out, size_t *out_len) {
        mol_builder_t b;
        MolBuilder_RawTransaction_init(&b);

        uint8_t ver[4] = {0,0,0,0};
        MolBuilder_RawTransaction_set_version(&b, ver, 4);

        /* cell_deps */
        mol_builder_t deps;
        MolBuilder_CellDepVec_init(&deps);
        for (int i = 0; i < _n_deps; i++) MolBuilder_CellDepVec_push(&deps, _dep_data[i]);
        mol_seg_res_t dr = MolBuilder_CellDepVec_build(deps);
        MolBuilder_RawTransaction_set_cell_deps(&b, dr.seg.ptr, dr.seg.len);
        free(dr.seg.ptr);

        /* header_deps (empty) */
        mol_builder_t hd;
        MolBuilder_Byte32Vec_init(&hd);
        mol_seg_res_t hdr = MolBuilder_Byte32Vec_build(hd);
        MolBuilder_RawTransaction_set_header_deps(&b, hdr.seg.ptr, hdr.seg.len);
        free(hdr.seg.ptr);

        /* inputs */
        mol_builder_t inputs;
        MolBuilder_CellInputVec_init(&inputs);
        for (int i = 0; i < _n_inputs; i++) MolBuilder_CellInputVec_push(&inputs, _input_data[i]);
        mol_seg_res_t ir = MolBuilder_CellInputVec_build(inputs);
        MolBuilder_RawTransaction_set_inputs(&b, ir.seg.ptr, ir.seg.len);
        free(ir.seg.ptr);

        /* outputs */
        mol_builder_t outputs;
        MolBuilder_CellOutputVec_init(&outputs);
        for (int i = 0; i < _n_outputs; i++)
            MolBuilder_CellOutputVec_push(&outputs, _output_data[i], _output_len[i]);
        mol_seg_res_t or_ = MolBuilder_CellOutputVec_build(outputs);
        MolBuilder_RawTransaction_set_outputs(&b, or_.seg.ptr, or_.seg.len);
        free(or_.seg.ptr);

        /* outputs_data */
        mol_builder_t od;
        MolBuilder_BytesVec_init(&od);
        for (int i = 0; i < _n_outputs; i++) {
            mol_seg_res_t obr = _buildBytes(_odata_data[i], _odata_len[i]);
            MolBuilder_BytesVec_push(&od, obr.seg.ptr, obr.seg.len);
            free(obr.seg.ptr);
        }
        mol_seg_res_t odr = MolBuilder_BytesVec_build(od);
        MolBuilder_RawTransaction_set_outputs_data(&b, odr.seg.ptr, odr.seg.len);
        free(odr.seg.ptr);

        mol_seg_res_t res = mol_table_builder_finalize(b);
        if (res.errno != MOL_OK) return WYMOL_ERR_BUILD;
        *out = res.seg.ptr;
        *out_len = res.seg.len;
        return WYMOL_OK;
    }

    void _buildWitnessArgs(const uint8_t *lock, size_t lock_len,
                            uint8_t **out, size_t *out_len) {
        /* lock field: BytesOpt containing Bytes */
        mol_builder_t lock_bytes_b;
        MolBuilder_BytesOpt_init(&lock_bytes_b);
        if (lock && lock_len > 0) {
            mol_seg_res_t lb = _buildBytes(lock, lock_len);
            MolBuilder_BytesOpt_set(&lock_bytes_b, lb.seg.ptr, lb.seg.len);
            free(lb.seg.ptr);
        }
        mol_seg_res_t lr  = MolBuilder_BytesOpt_build(lock_bytes_b);
        mol_seg_res_t e1  = _buildBytesOptEmpty();
        mol_seg_res_t e2  = _buildBytesOptEmpty();

        mol_builder_t wa;
        MolBuilder_WitnessArgs_init(&wa);
        MolBuilder_WitnessArgs_set_lock(&wa, lr.seg.ptr, lr.seg.len);
        MolBuilder_WitnessArgs_set_input_type(&wa, e1.seg.ptr, e1.seg.len);
        MolBuilder_WitnessArgs_set_output_type(&wa, e2.seg.ptr, e2.seg.len);
        free(lr.seg.ptr); free(e1.seg.ptr); free(e2.seg.ptr);

        mol_seg_res_t wa_res = mol_table_builder_finalize(wa);
        *out = wa_res.seg.ptr;
        *out_len = wa_res.seg.len;
    }
};
