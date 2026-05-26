const fs = require('fs');
const path = require('path');
const readline = require('readline');

const CSV_DIR = path.join(__dirname, 'tw_world');
const OUTPUT_FILE = path.join(__dirname, 'tw_world_upsert.sql');

const TABLE_CONFIG = {
  'locales_spell.csv': { table: 'locales_spell', pk: 'entry' },
  'spell_template.csv': { table: 'spell_template', pk: 'entry' },
  'creature_display_info_addon.csv': { table: 'creature_display_info_addon', pk: 'display_id' },
};

const BATCH_SIZE = 200;

function escapeValue(val) {
  if (val === null || val === undefined || val === '') return 'NULL';
  // If the value looks like a number, don't quote it
  if (/^-?\d+(\.\d+)?$/.test(val)) return val;
  // Escape SQL string
  return "'" + val.replace(/\\/g, '\\\\').replace(/'/g, "\\'") + "'";
}

/**
 * Parse a CSV line respecting quoted fields (handles commas and newlines inside quotes).
 */
function parseCSVLine(line) {
  const result = [];
  let current = '';
  let inQuote = false;

  for (let i = 0; i < line.length; i++) {
    const ch = line[i];
    if (ch === '"') {
      if (inQuote && line[i + 1] === '"') {
        // Escaped quote inside quoted field
        current += '"';
        i++;
      } else {
        inQuote = !inQuote;
      }
    } else if (ch === ',' && !inQuote) {
      result.push(current);
      current = '';
    } else {
      current += ch;
    }
  }
  result.push(current);
  return result;
}

async function parseCSV(filePath) {
  return new Promise((resolve, reject) => {
    const rows = [];
    let headers = null;
    let buffer = '';

    const stream = fs.createReadStream(filePath, { encoding: 'utf8' });

    stream.on('data', (chunk) => {
      buffer += chunk;
    });

    stream.on('end', () => {
      // Split into lines respecting quoted fields with embedded newlines
      const lines = [];
      let line = '';
      let inQuote = false;
      for (let i = 0; i < buffer.length; i++) {
        const ch = buffer[i];
        if (ch === '"') {
          if (inQuote && buffer[i + 1] === '"') {
            line += '"';
            i++;
          } else {
            inQuote = !inQuote;
            line += ch;
          }
        } else if ((ch === '\n' || ch === '\r') && !inQuote) {
          if (ch === '\r' && buffer[i + 1] === '\n') i++;
          if (line.trim()) lines.push(line);
          line = '';
        } else {
          line += ch;
        }
      }
      if (line.trim()) lines.push(line);

      for (const l of lines) {
        const fields = parseCSVLine(l);
        if (!headers) {
          headers = fields.map(h => h.replace(/^"|"$/g, ''));
        } else {
          const row = {};
          headers.forEach((h, i) => {
            row[h] = fields[i] !== undefined ? fields[i].replace(/^"|"$/g, '') : '';
          });
          rows.push(row);
        }
      }

      resolve({ headers, rows });
    });

    stream.on('error', reject);
  });
}

function generateUpsertSQL(tableName, headers, rows) {
  const cols = headers.map(h => `\`${h}\``).join(', ');
  const updateCols = headers
    .map(h => `\`${h}\`=VALUES(\`${h}\`)`)
    .join(',\n    ');

  const chunks = [];
  for (let i = 0; i < rows.length; i += BATCH_SIZE) {
    const batch = rows.slice(i, i + BATCH_SIZE);
    const values = batch
      .map(row => '(' + headers.map(h => escapeValue(row[h])).join(', ') + ')')
      .join(',\n  ');

    chunks.push(
      `INSERT INTO \`${tableName}\` (${cols})\nVALUES\n  ${values}\nON DUPLICATE KEY UPDATE\n    ${updateCols};`
    );
  }
  return chunks.join('\n\n');
}

async function main() {
  const out = fs.createWriteStream(OUTPUT_FILE, { encoding: 'utf8' });
  out.write('-- Auto-generated UPSERT SQL\n-- Generated: ' + new Date().toISOString() + '\n\n');
  out.write('SET NAMES utf8mb4;\n\n');

  for (const [filename, config] of Object.entries(TABLE_CONFIG)) {
    const filePath = path.join(CSV_DIR, filename);
    console.log(`Parsing ${filename}...`);
    const { headers, rows } = await parseCSV(filePath);
    console.log(`  ${rows.length} rows, ${headers.length} columns`);

    out.write(`-- ============================================================\n`);
    out.write(`-- Table: ${config.table}\n`);
    out.write(`-- ============================================================\n\n`);

    const sql = generateUpsertSQL(config.table, headers, rows);
    out.write(sql);
    out.write('\n\n');
    console.log(`  SQL written for ${config.table}`);
  }

  out.end();
  console.log(`\nDone! Output: ${OUTPUT_FILE}`);
}

main().catch(console.error);
