import React, {useEffect, useRef, useState} from 'react';
import Layout from '@theme/Layout';
import CodeBlock from '@theme/CodeBlock';
import {useLocation} from '@docusaurus/router';
import {AlertCircle, Check, Copy, Download, Link2, Search, X} from 'lucide';
import data from '../../data/benchmarks/results.json';
import styles from './benchmarks.module.css';

const suites = {applications: 'Applications', npbench: 'NPBench'};
const tools = ['codon', 'python', 'pypy', 'cpp', 'numba'];
const states = {timeout: 'Timed out', 'compile-error': 'Compile error', error: 'Execution failed'};
const number = new Intl.NumberFormat('en-US', {maximumSignificantDigits: 4});
const measuredCount = data.cases.reduce((count, item) => count + Object.values(item.results).filter(isMeasured).length, 0);

function isMeasured(result) {
  return Number.isFinite(result?.median);
}

function Icon({icon}) {
  return <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
    {icon.map(([tag, attributes], index) => React.createElement(tag, {...attributes, key: index}))}
  </svg>;
}

function version(tool) {
  const text = data.versions[tool === 'cpp' ? 'clang' : tool];
  if (tool === 'pypy') return text.match(/PyPy ([\d.]+)/)?.[1] || text;
  return text.match(/\d+\.\d+\.\d+/)?.[0] || text;
}

function duration(seconds) {
  if (seconds === 0) return '0 s';
  if (seconds >= 1) return `${number.format(seconds)} s`;
  if (seconds >= 0.001) return `${number.format(seconds * 1000)} ms`;
  return `${number.format(seconds * 1000000)} us`;
}

function TabList({label, options, value, onChange}) {
  function navigate(event, index) {
    const direction = {ArrowRight: 1, ArrowLeft: -1}[event.key];
    const next = event.key === 'Home' ? 0 : event.key === 'End' ? options.length - 1 : direction ? (index + direction + options.length) % options.length : null;
    if (next === null) return;
    event.preventDefault();
    onChange(options[next].value);
    event.currentTarget.parentElement.querySelectorAll('[role="tab"]')[next].focus();
  }
  return <div className={styles.tabs} role="tablist" aria-label={label}>
    {options.map((option, index) => <button key={option.value} type="button" role="tab" aria-selected={value === option.value}
      tabIndex={value === option.value ? 0 : -1} onClick={() => onChange(option.value)} onKeyDown={event => navigate(event, index)}>{option.label}</button>)}
  </div>;
}

function Chart({benchmark, mode, baseline, logarithmic}) {
  const entries = tools.filter(tool => benchmark.results[tool]).map(tool => [tool, benchmark.results[tool]]);
  const reference = benchmark.results[baseline];
  const valid = entries.filter(([, result]) => isMeasured(result));
  const values = valid.flatMap(([, result]) => mode === 'time' ? [result.minimum, result.maximum] : [reference.median / result.maximum, reference.median / result.minimum]);
  const minimum = logarithmic ? 10 ** Math.floor(Math.log10(Math.min(...values))) : 0;
  const maximum = values.length ? (logarithmic ? 10 ** Math.ceil(Math.log10(Math.max(...values) * 1.01)) : Math.max(...values) * 1.08) : 1;
  function position(value) {
    return logarithmic ? 100 * Math.log(value / minimum) / Math.log(maximum / minimum) : 100 * value / maximum;
  }
  const format = value => mode === 'time' ? duration(value) : `${number.format(value)}x`;
  const ticks = Array.from({length: 5}, (_, index) => logarithmic ? minimum * (maximum / minimum) ** (index / 4) : maximum * index / 4);
  return <figure className={styles.chart} aria-label={`${benchmark.title}: ${mode === 'time' ? 'median runtime, lower is better' : 'speedup, higher is better'}`}>
    <div className={styles.chartHeading}><span>{mode === 'time' ? 'Median wall time' : `Speedup relative to ${reference?.label || baseline}`}</span><span>{mode === 'time' ? 'Lower is better' : 'Higher is better'}</span></div>
    {entries.map(([tool, result]) => {
      const okay = isMeasured(result);
      const median = okay ? (mode === 'time' ? result.median : reference.median / result.median) : 0;
      const low = okay ? (mode === 'time' ? result.minimum : reference.median / result.maximum) : 0;
      const high = okay ? (mode === 'time' ? result.maximum : reference.median / result.minimum) : 0;
      return <div className={styles.chartRow} key={tool} data-tool={tool}>
        <div className={styles.chartLabel}><strong>{result.label}</strong><span>{version(tool)}</span></div>
        {okay ? <>
          <div className={styles.track} aria-hidden="true">
            <div className={styles.bar} style={{width: `${position(median)}%`}} />
            <div className={styles.range} style={{left: `${position(low)}%`, width: `${position(high) - position(low)}%`}} />
          </div>
          <div className={styles.chartValue}><strong>{format(median)}</strong><span>{format(low)} - {format(high)}</span></div>
        </> : <div className={styles.withheld}><Icon icon={AlertCircle} /><span><strong>{states[result.status] || 'Unavailable'}</strong><small>{result.status === 'timeout' ? `Process exceeded ${result.timeoutSeconds} seconds.` : result.reason || 'No comparable result was recorded.'}</small></span></div>}
      </div>;
    })}
    {valid.length > 0 && <div className={styles.axis} aria-hidden="true"><div /><div>{ticks.map((tick, index) => <span key={index} style={{left: `${index * 25}%`}}>{format(tick)}</span>)}</div><div /></div>}
    <figcaption>Median of {data.sampleCount} measured runs. Whiskers show the observed minimum and maximum, not a confidence interval.{logarithmic && ' Logarithmic axis.'}
      {benchmark.outputNote && <span className={styles.resultNote}>{benchmark.outputNote}</span>}
      {benchmark.threadingNote && <span className={styles.resultNote}>{benchmark.threadingNote}</span>}
    </figcaption>
  </figure>;
}

function Setup({benchmark, tool, result}) {
  const native = tool === 'codon';
  return <div className={styles.setup}>
    <dl className={styles.facts}>
      <div><dt>Implementation</dt><dd>{result.label} {version(tool)}{tool === 'numba' && ` / CPython ${data.versions.python}`}</dd></div>
      <div><dt>Configuration</dt><dd>{benchmark.suite === 'npbench' ? 'NPBench L (largest standard preset)' : 'Full application workload'}</dd></div>
      <div><dt>Timing boundary</dt><dd>{benchmark.policy}</dd></div>
      <div><dt>Output check</dt><dd>{result.validation?.status === 'passed' ? 'Passed' : result.status === 'invalid' ? 'Output differs between runtimes' : isMeasured(result) ? 'Not recorded' : 'Not completed'}{result.validation?.elements != null && `; ${number.format(result.validation.elements)} array elements checked`}</dd></div>
      {result.parallelThreads != null && <div><dt>OpenMP threads</dt><dd>{result.parallelThreads}{result.parallelThreads > 1 ? ' (explicit in the source)' : ' (environment setting)'}</dd></div>}
      <div><dt>Source SHA-256</dt><dd className={styles.hash}>{result.sourceSha256}</dd></div>
    </dl>
    {Object.keys(benchmark.parameters).length > 0 && <><h3>Parameters</h3><div className={styles.parameters}>{Object.entries(benchmark.parameters).map(([name, value]) => <span key={name}><code>{name}</code> {String(value)}</span>)}</div></>}
    {benchmark.arguments.length > 0 && <><h3>Arguments</h3><code className={styles.wrapCode}>{benchmark.arguments.join(' ')}</code></>}
    {Object.keys(benchmark.inputs).length > 0 && <details className={styles.details}><summary>Input arrays and scalar values</summary><div className={styles.tableScroll}><table><thead><tr><th>Argument</th><th>Type</th><th>Shape / value</th></tr></thead><tbody>{Object.entries(benchmark.inputs).map(([name, input]) => <tr key={name}><td><code>{name}</code></td><td>{input.dtype || 'scalar'}</td><td>{input.shape?.join(' x ') || String(input.value)}</td></tr>)}</tbody></table></div></details>}
    {result.compileCommand && <><h3>Build command</h3><pre className={styles.command}>{result.compileCommand.map(argument => /\s/.test(argument) ? JSON.stringify(argument) : argument).join(' ')}</pre></>}
    {result.runtime && <><h3>Numerical libraries</h3><ul>{result.runtime.map((pool, index) => <li key={index}>{pool.internal_api} {pool.version}, {pool.num_threads} thread{pool.num_threads === 1 ? '' : 's'}<br /><code className={styles.wrapCode}>{pool.filepath}</code></li>)}</ul></>}
    {benchmark.suite === 'npbench' && native && <details className={styles.details}><summary>Memory observations</summary><p className={styles.small}>{result.memoryAudit}. Reset buffers retain their addresses; collection and reset are outside kernel timing.</p>{result.gc && <p className={styles.small}>Post-collection live growth: {number.format(result.gc.liveGrowthBytes / 1048576)} MiB. Full observations are retained in the raw report.</p>}</details>}
    {(result.adaptations || []).length > 0 && <><h3>Recorded adaptations</h3><ul>{result.adaptations.map(note => <li key={note}>{note}</li>)}</ul></>}
    {result.workerUrl && <a href={result.workerUrl} download>Download the measurement harness (paths redacted)</a>}
  </div>;
}

function SourcePane({benchmark, tool, view, setTool, setView}) {
  const result = benchmark.results[tool];
  const [source, setSource] = useState({url: '', text: '', error: false});
  const [copyState, setCopyState] = useState('');
  const copyGeneration = useRef(0);
  const url = result.displaySourceUrl || result.sourceUrl;
  useEffect(() => {
    const controller = new AbortController();
    copyGeneration.current++;
    setCopyState('');
    fetch(url, {signal: controller.signal}).then(response => {
      if (!response.ok || response.headers.get('content-type')?.includes('text/html')) throw new Error('Source unavailable');
      return response.text();
    }).then(text => setSource({url, text, error: false})).catch(error => {
      if (error.name !== 'AbortError') setSource({url, text: '', error: true});
    });
    return () => { controller.abort(); copyGeneration.current++; };
  }, [url]);
  async function copySource() {
    const generation = ++copyGeneration.current;
    try { await navigator.clipboard.writeText(source.text); if (generation === copyGeneration.current) setCopyState('Copied'); }
    catch { if (generation === copyGeneration.current) setCopyState('Copy unavailable'); }
  }
  return <section className={styles.inspector} aria-label="Benchmark implementation">
    <TabList label="Benchmark details" options={[{value: 'source', label: 'Source'}, {value: 'setup', label: 'Setup'}, {value: 'samples', label: 'Samples'}]} value={view} onChange={setView} />
    <div className={styles.toolRow}>
      <TabList label="Implementation" options={tools.filter(key => benchmark.results[key]).map(key => ({value: key, label: benchmark.results[key].label}))} value={tool} onChange={setTool} />
      <span className={styles.toolVersion}>{result.label} {version(tool)}</span>
    </div>
    <div className={styles.detailPanel} role="tabpanel" aria-label={`${result.label} ${view}`}>
      {view === 'source' && <>
        <div className={styles.sourceToolbar}><span>{result.displaySourceUrl ? 'Kernel excerpt' : 'Executed source'}</span><div className={styles.actions}>
          <span role="status">{copyState}</span>
          <button type="button" title="Copy displayed source" aria-label="Copy displayed source" onClick={copySource} disabled={source.url !== url || !source.text}><Icon icon={copyState === 'Copied' ? Check : Copy} /></button>
          <a href={result.sourceUrl} download title={result.displaySourceUrl ? 'Download full executed library' : 'Download executed source'} aria-label="Download executed source"><Icon icon={Download} /></a>
        </div></div>
        <div className={styles.code}>{source.url !== url ? <p role="status">Loading source...</p> : source.error ? <p role="alert">Source could not be loaded. <a href={result.sourceUrl}>Open source file</a></p> : <CodeBlock language={result.language} showLineNumbers>{source.text}</CodeBlock>}</div>
        {result.displaySourceUrl && <p className={styles.sourceNote}>Excerpt from the executed native library. <a href={result.sourceUrl} download>Full library</a></p>}
      </>}
      {view === 'setup' && <Setup benchmark={benchmark} tool={tool} result={result} />}
      {view === 'samples' && <div className={styles.samplePane}><p className={styles.small}>Recorded wall time in seconds. Warm-up is excluded; incomplete runs have no reported timing.</p><div className={styles.tableScroll}><table><thead><tr><th>Implementation</th>{Array.from({length: data.sampleCount}, (_, index) => <th key={index}>Run {index + 1}</th>)}<th>Median</th></tr></thead><tbody>{tools.filter(key => benchmark.results[key]).map(key => {
        const item = benchmark.results[key];
        return <tr key={key}><th scope="row">{item.label}</th>{isMeasured(item) ? <>{item.samples.map((sample, index) => <td key={index}>{sample.toPrecision(6)}</td>)}<td>{item.median.toPrecision(6)}</td></> : <td colSpan={data.sampleCount + 1}>{states[item.status] || 'Unavailable'}</td>}</tr>;
      })}</tbody></table></div></div>}
    </div>
  </section>;
}

function Methodology() {
  const environment = data.environment;
  return <details className={styles.methodology} id="methodology"><summary>Methodology and reproducibility</summary>
    <div className={styles.methodGrid}><section><h2>One machine, two timing scopes</h2><p>{environment.cpu}, {environment.memoryBytes / 1073741824} GiB RAM, macOS {environment.os} ({environment.macosBuild}), {environment.logicalCpus} logical CPUs. OpenMP and numerical libraries default to one worker. Codon's Fannkuch explicitly requests four OpenMP threads; its parallel Primes loop uses the one-thread default. Native compilation and input preparation finished before the serial timing sweep.</p>
      <p>Applications retain their original timing boundaries. Each measurement uses a fresh process, so allocation and any PyPy JIT work inside the timer are included. One preliminary process is excluded. NPBench uses one process per implementation, one warm-up, then three kernel-only measurements; Numba specialization, input reset, and output serialization are excluded.</p>
      <p>These are software-stack comparisons, not isolated compiler comparisons. Native Codon links Apple Accelerate; NumPy and Numba use the libraries recorded under Setup. Upstream Numba flags, including fast math, are retained. Results describe this hardware and these inputs, not a universal ranking.</p>
      <h3>Memory and output checks</h3><p>NPBench uses identical initialized inputs and persistent pristine/working arrays. Working arrays are reset in place, with garbage collection outside the timer. A separate reset audit checks allocation and address stability. Live-heap observations are retained in the raw report and do not determine whether completed timings are displayed.</p>
      <p>NPBench output checks use upstream pointwise tolerances or a relative L2-error threshold, with exact integer checks. Application values are compared with CPython; Chaos uses an exact PPM check. Completed timings are included when outputs differ, with a brief note on the affected benchmark. Rounding and the order of floating-point operations can affect iterative calculations; the original check results remain available in the raw report.</p>
      <p>The process timeout is {data.processTimeoutSeconds} seconds, including warm-up and output writing. PyPy's Fannkuch run timed out and has no reported timing.</p>
    </section><section><h2>Recorded environment</h2><dl className={styles.environment}>{Object.entries({Codon: version('codon'), CPython: version('python'), PyPy: `${version('pypy')} (Python 3.12.15)`, Clang: version('cpp'), NumPy: data.versions.numpy, Numba: data.versions.numba, SciPy: data.versions.scipy, 'macOS SDK': environment.sdk}).map(([name, value]) => <div key={name}><dt>{name}</dt><dd>{value}</dd></div>)}</dl>
      <p>Clang uses the matching macOS SDK libc++ headers and system runtime. The CUDA-only Codon Mandelbrot variant is not measured on this host; the existing CPU implementation is used instead.</p>
      <h3>Source and inputs</h3><p>Codon commit <a href={`https://github.com/exaloop/codon/tree/${environment.codonCommit}`}><code>{environment.codonCommit.slice(0, 12)}</code></a>; local binary and source fingerprints are included in the raw report. NPBench commit <a href={`https://github.com/spcl/npbench/tree/${environment.npbenchCommit}`}><code>{environment.npbenchCommit.slice(0, 12)}</code></a>, always the largest standard <code>L</code> preset.</p>
      <p>TAQ uses the first 10 million lines of {data.datasets.taq.source}. Word count uses {number.format(data.datasets.words.bytes / 1048576)} MiB of {data.datasets.words.source}, ending at a complete line. Dataset hashes and array dimensions are recorded; external input files are not redistributed.</p>
      <p>The Numba MLP source corrects an upstream softmax maximum-axis error. Other compatibility adaptations are listed per implementation. Exact executed kernel/application sources and raw samples accompany the results.</p>
      <div className={styles.resourceLinks}><a href="/benchmarks/reproduction.md" download>Reproduction guide</a><a href="/benchmarks/raw-results.json" download>Raw results and provenance</a><a href="/benchmarks/NPBench-LICENSE.txt">NPBench license</a></div>
    </section></div>
  </details>;
}

export default function Benchmarks() {
  const location = useLocation();
  const [selection, setSelection] = useState({benchmark: data.cases[0].id, tool: 'codon', view: 'source'});
  const [query, setQuery] = useState('');
  const [suite, setSuite] = useState('all');
  const [mode, setMode] = useState('time');
  const [baseline, setBaseline] = useState('python');
  const [logarithmic, setLogarithmic] = useState(false);
  const [shareState, setShareState] = useState('');
  useEffect(() => {
    const params = new URLSearchParams(location.search);
    const benchmark = data.cases.find(item => item.id === params.get('benchmark')) || data.cases[0];
    const tool = Object.hasOwn(benchmark.results, params.get('tool')) ? params.get('tool') : Object.keys(benchmark.results)[0];
    const view = ['source', 'setup', 'samples'].includes(params.get('view')) ? params.get('view') : 'source';
    setSelection({benchmark: benchmark.id, tool, view});
    setShareState('');
  }, [location.search]);
  const benchmark = data.cases.find(item => item.id === selection.benchmark);
  const validTools = tools.filter(tool => isMeasured(benchmark.results[tool]));
  const actualBaseline = validTools.includes(baseline) ? baseline : validTools[0];
  const filtered = data.cases.filter(item => (suite === 'all' || item.suite === suite) && `${item.name} ${item.title}`.toLowerCase().includes(query.toLowerCase().trim()));
  function select(changes) {
    const next = {...selection, ...changes};
    const target = data.cases.find(item => item.id === next.benchmark);
    if (!target.results[next.tool]) next.tool = Object.keys(target.results)[0];
    setSelection(next);
    setShareState('');
    window.history.replaceState(window.history.state, '', `${location.pathname}?${new URLSearchParams(next)}${location.hash}`);
  }
  async function share() {
    try { await navigator.clipboard.writeText(window.location.href); setShareState('Link copied'); }
    catch { setShareState('Copy unavailable'); }
  }
  return <Layout title="Benchmarks" description="Reproducible Codon, Python, PyPy, C++ and Numba performance results with source code, raw measurements and setup details.">
    <main className={styles.page}>
      <header className={styles.header}><div><p className={styles.eyebrow}>Performance / {data.date}</p><h1>Benchmarks</h1></div><div className={styles.headerMeta}><span>{data.environment.cpu}</span><span>{data.cases.length} workloads</span><span>{measuredCount} timings</span></div></header>
      <div className={styles.workspace}>
        <aside className={styles.catalogue} aria-label="Benchmark catalogue"><div className={styles.catalogueHeading}><h2>Workloads</h2><span>{filtered.length}</span></div>
          <label className={styles.search}><Icon icon={Search} /><input type="search" value={query} onChange={event => setQuery(event.target.value)} placeholder="Search workloads" aria-label="Search workloads" />{query && <button type="button" onClick={() => setQuery('')} aria-label="Clear workload search" title="Clear search"><Icon icon={X} /></button>}</label>
          <label className={styles.suiteSelect}><span>Suite</span><select aria-label="Benchmark suite" value={suite} onChange={event => setSuite(event.target.value)}><option value="all">All suites</option><option value="applications">Applications (13)</option><option value="npbench">NPBench (53)</option></select></label>
          <nav className={styles.workloadList} aria-label="Workloads">{Object.entries(suites).map(([key, label]) => {
            const items = filtered.filter(item => item.suite === key);
            return items.length > 0 && <div key={key}><h3>{label}</h3>{items.map(item => <a key={item.id} href={`${location.pathname}?${new URLSearchParams({...selection, benchmark: item.id})}`} title={item.name} onClick={event => {
              if (event.button !== 0 || event.metaKey || event.ctrlKey || event.shiftKey || event.altKey) return;
              event.preventDefault();
              select({benchmark: item.id});
            }} className={selection.benchmark === item.id ? styles.selectedWorkload : undefined} aria-current={selection.benchmark === item.id ? 'true' : undefined}><span>{item.name}</span></a>)}</div>;
          })}{!filtered.length && <p className={styles.empty}>No matching workloads.</p>}</nav>
        </aside>
        <div className={styles.main}>
          <label className={styles.mobileSelector}><span>Workload</span><select aria-label="Workload" value={benchmark.id} onChange={event => select({benchmark: event.target.value})}>{Object.entries(suites).map(([key, label]) => <optgroup key={key} label={label}>{data.cases.filter(item => item.suite === key).map(item => <option key={item.id} value={item.id}>{item.name}</option>)}</optgroup>)}</select></label>
          <div className={styles.titleRow}><div><p className={styles.eyebrow}>{suites[benchmark.suite]} / {benchmark.preset === 'L' ? 'L preset' : 'Full input'}</p><h2>{benchmark.title}</h2><p className={styles.subtitle}>{benchmark.suite === 'npbench' ? Object.entries(benchmark.parameters).map(([name, value]) => `${name} = ${value}`).join(' / ') : benchmark.arguments.length ? `Arguments: ${benchmark.arguments.join(' ')}` : 'Original application parameters'}</p></div><div className={styles.actions}><span role="status">{shareState}</span><button type="button" onClick={share} title="Copy benchmark link" aria-label="Copy benchmark link"><Icon icon={shareState === 'Link copied' ? Check : Link2} /></button><a href="/benchmarks/results.json" download title="Download results JSON" aria-label="Download results JSON"><Icon icon={Download} /></a></div></div>
          <div className={styles.chartControls}><div className={styles.segmented} role="group" aria-label="Chart metric"><button type="button" aria-pressed={mode === 'time'} onClick={() => setMode('time')}>Runtime</button><button type="button" aria-pressed={mode === 'speedup'} onClick={() => setMode('speedup')} disabled={!validTools.length}>Speedup</button></div><div className={styles.chartOptions}>{mode === 'speedup' && <label>Baseline <select aria-label="Speedup baseline" value={actualBaseline} onChange={event => setBaseline(event.target.value)}>{validTools.map(tool => <option key={tool} value={tool}>{benchmark.results[tool].label}</option>)}</select></label>}<label><input type="checkbox" checked={logarithmic} onChange={event => setLogarithmic(event.target.checked)} /> Log scale</label></div></div>
          <Chart benchmark={benchmark} mode={validTools.length ? mode : 'time'} baseline={actualBaseline} logarithmic={logarithmic} />
          {benchmark.name === 'sum' && benchmark.suite === 'applications' && <p className={styles.notice}>This loop can be simplified algebraically by a compiler. Its speedup is specific to this optimization, not representative of general application performance.</p>}
          <SourcePane benchmark={benchmark} tool={selection.tool} view={selection.view} setTool={tool => select({tool})} setView={view => select({view})} />
        </div>
      </div>
      <Methodology />
      <div className={styles.bottomLinks}><span>Measured {data.date}. No historical or pilot timings included.</span><a href="/benchmarks/raw-results.json" download>Download raw results</a></div>
    </main>
  </Layout>;
}
