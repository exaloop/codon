(() => {
  let searchIndex = null;

  function normalize(value) {
    return value
      .toLowerCase()
      .replace(/[_\-.]+/g, " ")
      .replace(/\s+/g, " ")
      .trim();
  }

  function tokens(value) {
    return normalize(value)
      .split(" ")
      .filter(Boolean);
  }

  function score(entry, query) {
    const rawQuery = query.toLowerCase().trim();
    const normalizedQuery = normalize(query);

    const name = entry.name.toLowerCase();
    const qualified = entry.qualified.toLowerCase();

    const normalizedName = normalize(entry.name);
    const normalizedQualified = normalize(entry.qualified);

    /*
     * Exact symbol-name match.
     *
     *     reset -> Counter.reset
     */
    if (name === rawQuery) {
      return 1000;
    }

    /*
     * Exact qualified-name match.
     *
     *     collections.Counter.reset
     */
    if (qualified === rawQuery) {
      return 950;
    }

    /*
     * Normalized exact matches.
     *
     *     bisect left -> bisect_left
     */
    if (normalizedName === normalizedQuery) {
      return 900;
    }

    if (normalizedQualified === normalizedQuery) {
      return 850;
    }

    /*
     * Symbol prefix.
     *
     *     bis -> bisect_left
     */
    if (name.startsWith(rawQuery)) {
      return 800;
    }

    if (normalizedName.startsWith(normalizedQuery)) {
      return 750;
    }

    /*
     * Qualified-name prefix.
     */
    if (qualified.startsWith(rawQuery)) {
      return 700;
    }

    if (normalizedQualified.startsWith(normalizedQuery)) {
      return 650;
    }

    /*
     * All query tokens appear somewhere in the qualified name.
     *
     *     counter reset
     *         -> collections.Counter.reset
     *
     *     collections counter
     *         -> collections.Counter
     */
    const queryTokens = tokens(query);

    if (
      queryTokens.length &&
      queryTokens.every(token =>
        normalizedQualified.includes(token)
      )
    ) {
      let result = 500;

      /*
       * Prefer matches whose tokens occur in the short symbol name.
       */
      for (const token of queryTokens) {
        if (normalizedName.includes(token)) {
          result += 20;
        }
      }

      return result;
    }

    /*
     * Basic substring matching.
     */
    if (name.includes(rawQuery)) {
      return 400;
    }

    if (normalizedName.includes(normalizedQuery)) {
      return 350;
    }

    if (qualified.includes(rawQuery)) {
      return 300;
    }

    if (normalizedQualified.includes(normalizedQuery)) {
      return 250;
    }

    return -1;
  }

  function kindLabel(kind) {
    switch (kind) {
      case "module":
        return "module";
      case "class":
        return "class";
      case "function":
        return "function";
      case "variable":
        return "variable";
      case "property":
        return "property";
      case "method":
        return "method";
      default:
        return kind || "";
    }
  }

  function search(index, query) {
    if (!query.trim()) {
      return [];
    }

    return index
      .map(entry => ({
        entry,
        score: score(entry, query),
      }))
      .filter(result => result.score >= 0)
      .sort((a, b) => {
        if (a.score !== b.score) {
          return b.score - a.score;
        }

        /*
         * Prefer shorter qualified names when scores tie.
         */
        if (
          a.entry.qualified.length !==
          b.entry.qualified.length
        ) {
          return (
            a.entry.qualified.length -
            b.entry.qualified.length
          );
        }

        return a.entry.qualified.localeCompare(
          b.entry.qualified
        );
      })
      .slice(0, 12)
      .map(result => result.entry);
  }

  function initApiSearch() {
    const input = document.getElementById(
      "api-search-input"
    );

    const results = document.getElementById(
      "api-search-results"
    );

    if (!input || !results) {
      return;
    }

    let selected = -1;

    async function loadIndex() {
      if (searchIndex !== null) {
        return searchIndex;
      }

      try {
        const response = await fetch(
          "./api-search-index.json"
        );

        if (!response.ok) {
          throw new Error(
            `HTTP ${response.status}`
          );
        }

        searchIndex = await response.json();

        return searchIndex;
      } catch (error) {
        console.error(
          "Unable to load API search index:",
          error
        );

        searchIndex = [];

        return searchIndex;
      }
    }

    function closeResults() {
      results.hidden = true;
      results.innerHTML = "";
      selected = -1;

      input.setAttribute(
        "aria-expanded",
        "false"
      );

      input.removeAttribute(
        "aria-activedescendant"
      );
    }

    function selectResult(index) {
      const items = Array.from(
        results.querySelectorAll(
          ".api-search-result"
        )
      );

      if (!items.length) {
        selected = -1;
        return;
      }

      if (index < 0) {
        index = items.length - 1;
      }

      if (index >= items.length) {
        index = 0;
      }

      selected = index;

      for (let i = 0; i < items.length; ++i) {
        const active = i === selected;

        items[i].classList.toggle(
          "is-selected",
          active
        );

        items[i].setAttribute(
          "aria-selected",
          active ? "true" : "false"
        );
      }

      const item = items[selected];

      input.setAttribute(
        "aria-activedescendant",
        item.id
      );

      item.scrollIntoView({
        block: "nearest",
      });
    }

    function render(entries) {
      results.innerHTML = "";
      selected = -1;

      if (!entries.length) {
        const empty = document.createElement(
          "div"
        );

        empty.className =
          "api-search-empty";

        empty.textContent =
          "No API symbols found.";

        results.appendChild(empty);
        results.hidden = false;

        input.setAttribute(
          "aria-expanded",
          "true"
        );

        return;
      }

      entries.forEach((entry, index) => {
        const link = document.createElement(
          "a"
        );

        link.id =
          `api-search-result-${index}`;

        link.className =
          "api-search-result";

        link.href = entry.url;
        link.setAttribute(
          "role",
          "option"
        );

        link.setAttribute(
          "aria-selected",
          "false"
        );

        const symbol =
          document.createElement("div");

        symbol.className =
          "api-search-result-symbol";

        const name =
          document.createElement("code");

        name.className =
          "api-search-result-name";

        name.textContent = entry.name;

        const kind =
          document.createElement("span");

        kind.className =
          `api-search-result-kind ` +
          `api-search-result-kind-${entry.kind}`;

        kind.textContent =
          kindLabel(entry.kind);

        symbol.appendChild(name);
        symbol.appendChild(kind);

        const qualified =
          document.createElement("code");

        qualified.className =
          "api-search-result-qualified";

        qualified.textContent =
          entry.qualified;

        link.appendChild(symbol);
        link.appendChild(qualified);

        link.addEventListener(
          "mousemove",
          () => {
            selectResult(index);
          }
        );

        results.appendChild(link);
      });

      results.hidden = false;

      input.setAttribute(
        "aria-expanded",
        "true"
      );
    }

    async function update() {
      const query = input.value.trim();

      if (!query) {
        closeResults();
        return;
      }

      const index = await loadIndex();

      /*
       * Make sure the query hasn't changed while the index
       * was being fetched.
       */
      if (query !== input.value.trim()) {
        return;
      }

      render(
        search(index, query)
      );
    }

    input.addEventListener(
      "input",
      update
    );

    input.addEventListener(
      "focus",
      () => {
        if (input.value.trim()) {
          update();
        }
      }
    );

    input.addEventListener(
      "keydown",
      event => {
        if (event.key === "ArrowDown") {
          event.preventDefault();

          if (results.hidden) {
            update();
          } else {
            selectResult(
              selected + 1
            );
          }
        }

        else if (event.key === "ArrowUp") {
          event.preventDefault();

          if (!results.hidden) {
            selectResult(
              selected - 1
            );
          }
        }

        else if (event.key === "Enter") {
          if (selected >= 0) {
            const items =
              results.querySelectorAll(
                ".api-search-result"
              );

            const item =
              items[selected];

            if (item) {
              event.preventDefault();
              item.click();
            }
          }
        }

        else if (event.key === "Escape") {
          closeResults();
          input.blur();
        }
      }
    );

    document.addEventListener(
      "click",
      event => {
        if (
          !input.contains(event.target) &&
          !results.contains(event.target)
        ) {
          closeResults();
        }
      }
    );

    /*
     * "/" focuses API search unless the user is already typing
     * in an input or textarea.
     */
    document.addEventListener(
      "keydown",
      event => {
        if (event.key !== "/") {
          return;
        }

        const target = event.target;

        if (
          target instanceof HTMLInputElement ||
          target instanceof HTMLTextAreaElement ||
          target.isContentEditable
        ) {
          return;
        }

        event.preventDefault();
        input.focus();
      }
    );

    /*
     * Start loading immediately rather than waiting for the
     * user's first keystroke.
     */
    loadIndex();
  }

  function initApiClassScroll() {
    document
      .querySelectorAll(".api-class-scroll-wrap")
      .forEach(wrapper => {
        const scroll =
          wrapper.querySelector(".api-class-scroll");

        const hint =
          wrapper.querySelector(".api-class-scroll-hint");

        if (!scroll || !hint) {
          return;
        }

        function update() {
          const atBottom =
            scroll.scrollTop +
              scroll.clientHeight >=
            scroll.scrollHeight - 2;

          hint.style.opacity =
            atBottom ? "0" : "1";
        }

        scroll.addEventListener(
          "scroll",
          update,
          { passive: true }
        );

        update();
      });
  }

  /*
   * Material's instant navigation can replace page contents without
   * performing a normal browser reload. document$ is Material's
   * page-change observable when available.
   */
  if (typeof document$ !== "undefined") {
    document$.subscribe(() => {
      initApiSearch();
      initApiClassScroll();
    });
  } else if (
    document.readyState === "loading"
  ) {
    document.addEventListener(
      "DOMContentLoaded",
      initApiSearch
    );
    document.addEventListener(
      "DOMContentLoaded",
      initApiClassScroll
    );
  } else {
    initApiSearch();
    initApiClassScroll();
  }
})();
