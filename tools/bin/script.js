(function() {
    const folderStorageKey = "vanir.docs.openFolders";
    const links = Array.from(document.querySelectorAll(".sb-file"));
    const fileBlocks = Array.from(document.querySelectorAll(".file-block"));
    const cards = Array.from(document.querySelectorAll(".symbol-card"));

    let suppressAutoOpenUntil = 0;
    let ticking = false;
    let currentFileId = "";
    let currentSymbolId = "";
    let lastScrolledSidebarFile = "";
    let pendingScrollTarget = "";

    function now() {
        return Date.now ? Date.now() : new Date().getTime();
    }

    function saveOpenFolders() {
        const open = Array.from(document.querySelectorAll(".sb-folder.open"))
            .map(el => el.dataset.folderPath || "")
            .filter(Boolean);

        try {
            localStorage.setItem(folderStorageKey, JSON.stringify(open));
        } catch (e) {}
    }

    function restoreOpenFolders() {
        let open = [];

        try {
            open = JSON.parse(localStorage.getItem(folderStorageKey) || "[]") || [];
        } catch (e) {}

        if (!Array.isArray(open) || open.length === 0) 
            return;

        const set = new Set(open);

        document.querySelectorAll(".sb-folder").forEach(folder => {
            const path = folder.dataset.folderPath || "";

            if (set.has(path)) {
                folder.classList.add("open");
            }
        });
    }

    function toggle(card) {
        card.classList.toggle("open");
    }

    function toggleFolder(folder) {
        folder.classList.toggle("open");
        saveOpenFolders();
    }

    function foldPathParts(path) {
        return String(path || "").split("/").filter(Boolean);
    }

    function openAncestorsFromPath(path) {
        const parts = foldPathParts(path);

        parts.pop();

        let accum = "";

        for (let i = 0; i < parts.length; i++) {
            accum = accum ? (accum + "/" + parts[i]) : parts[i];
            const folder = document.querySelector(
                '.sb-folder[data-folder-path="' + accum.replace(/"/g, '\\"') + '"]'
            );

            if (folder) {
                folder.classList.add("open");
            }
        }

        saveOpenFolders();
    }

    function setActiveLinkById(id, doScrollSidebar) {
        if (!id) 
                return;

        links.forEach(link => {
            link.classList.toggle("active", link.dataset.file === id);
        });

        const link = document.querySelector(
            '.sb-file[data-file="' + id.replace(/"/g, '\\"') + '"]'
        );

        if (!link) 
                return;

        if (doScrollSidebar && lastScrolledSidebarFile !== id) {
            const sidebar = document.getElementById("sidebar");
            const r = link.getBoundingClientRect();
            const sr = sidebar.getBoundingClientRect();

            if (r.top < sr.top || r.bottom > sr.bottom) {
                lastScrolledSidebarFile = id;

                link.scrollIntoView({ block: "center", behavior: "smooth" });
            }
        }
    }

    function setActiveSymbolById(id) {
        cards.forEach(card => {
            card.classList.toggle("active-symbol", card.id === id);
        });
    }

    function visibleSections() {
        return fileBlocks.filter(block => block.style.display !== "none");
    }

    function currentVisibleFileBlock() {
        const visible = visibleSections();
        let best = null;
        let bestTop = -1e9;

        const header = document.querySelector(".meta-bar");
        const threshold = header ? header.offsetHeight + 20 : 140;

        visible.forEach(block => {
            const rect = block.getBoundingClientRect();

            if (rect.bottom < 0 || rect.top > window.innerHeight) 
                    return;

            if (rect.top <= threshold && rect.top > bestTop) {
                bestTop = rect.top;
                best = block;
            }
        });

        if (best) 
            return best;

        return visible[0] || null;
    }

    function currentVisibleSymbol(block) {
        if (!block) 
                return null;

        const visibleCards = Array.from(
            block.querySelectorAll(".symbol-card:not(.hidden)")
        );

        let best = null;
        let bestTop = -1e9;
        const threshold = 160;

        visibleCards.forEach(card => {
            const rect = card.getBoundingClientRect();

            if (rect.bottom < 0 || rect.top > window.innerHeight) return;

            if (rect.top <= threshold && rect.top > bestTop) {
                bestTop = rect.top;
                best = card;
            }
        });

        if (best) return best;

        return visibleCards.find(card => {
            const rect = card.getBoundingClientRect();

            return rect.bottom > 0 && rect.top < window.innerHeight;
        }) || null;
    }

    function openSymbolCard(card) {
        if (!card) return;

        card.classList.add("open");
    }

    function scrollToId(id) {
        const el = document.getElementById(id);

        if (el) {
            el.scrollIntoView({ block: "start", behavior: "auto" });
        }
    }

    function activateFromHash() {
        const hash = location.hash ? location.hash.slice(1) : "";

        if (!hash) {
            requestSync(false);

            return;
        }

        const [fileId, symbolId] = hash.split("::");

        suppressAutoOpenUntil = now() + 800;

        if (symbolId) {
            const card = document.getElementById(symbolId);

            if (card) {
                openSymbolCard(card);
                scrollToId(symbolId);
                setActiveSymbolById(symbolId);
            }
        }

        if (fileId) {
            pendingScrollTarget = fileId; // hopefully prevent rubberband

            setActiveLinkById(fileId, false);

            if (!symbolId) {
                scrollToId(fileId);
            }
        }

        requestSync(false);
    }

    function doSearch(q) {
        q = String(q || "").toLowerCase().trim();

        cards.forEach(card => {
            const hay = card.dataset.search || "";
            const match = !q || q.split(/\s+/).every(t => hay.includes(t));

            card.classList.toggle("hidden", !match);
        });

        fileBlocks.forEach(block => {
            const visible = block.querySelectorAll(".symbol-card:not(.hidden)");

            block.style.display = (!q || visible.length > 0) ? "" : "none";
        });

        requestSync(false);
    }

    function syncViewport(fromScroll) {
        ticking = false;

        if (fromScroll && pendingScrollTarget) {
            const target = document.getElementById(pendingScrollTarget);

            if (target) {
                const rect = target.getBoundingClientRect();
                const header = document.querySelector(".meta-bar");
                const threshold = header ? header.offsetHeight + 20 : 140;

                if (rect.top > threshold) {
                    return; // wait until target reaches top
                }
            }

            pendingScrollTarget = "";
        }

        const block = currentVisibleFileBlock();
        if (!block) return;

        const fileId = block.id || "";
        const card = currentVisibleSymbol(block);
        const symbolId = card ? card.id : "";

        if (fileId) {
            const fileChanged = fileId !== currentFileId;
            currentFileId = fileId;

            setActiveLinkById(fileId, fromScroll);

            if (fromScroll && fileChanged) {
                const p = block.dataset.filePath || "";
                if (p && now() >= suppressAutoOpenUntil) {
                    openAncestorsFromPath(p);
                }
            }
        }

        if (symbolId) {
            currentSymbolId = symbolId;

            if (!fromScroll) {
                setActiveSymbolById(symbolId);
            }
        } else {
            currentSymbolId = "";
        }
    }

    function requestSync(fromScroll) {
        if (ticking) 
                return;

        ticking = true;

        requestAnimationFrame(() => syncViewport(!!fromScroll));
    }

    function updateHashFromClick(fileId, symbolId) {
        const next = symbolId ? (fileId + "::" + symbolId) : fileId;

        if (location.hash.slice(1) !== next) {
            history.replaceState(null, "", "#" + next);
        }
    }

    function bindCardClicks() {
        cards.forEach(card => {
            const header = card.querySelector(".symbol-header");

            if (!header) 
                    return;

            header.addEventListener("click", e => {
                e.preventDefault();
                e.stopPropagation();

                card.classList.toggle("open");
                suppressAutoOpenUntil = now() + 800;

                setActiveSymbolById(card.id);
                updateHashFromClick(card.dataset.file || "", card.id || "");
            });
        });
    }

    function bindSidebarLinks() {
        links.forEach(link => {
            link.addEventListener("click", function(e) {
                e.preventDefault();

                const id = this.dataset.file || "";
                const el = document.getElementById(id);

                suppressAutoOpenUntil = now() + 900;
                pendingScrollTarget = id;

                if (el) {
                    el.scrollIntoView({ behavior: "smooth", block: "start" });
                    setActiveLinkById(id, true);
                    updateHashFromClick(id, "");
                }
            });
        });
    }

    function bindScroll() {
        window.addEventListener("scroll", () => requestSync(true), { passive: true });
        window.addEventListener("resize", () => requestSync(false));
    }

    function bindHash() {
        window.addEventListener("hashchange", activateFromHash);
    }

    function init() {
        restoreOpenFolders();
        bindCardClicks();
        bindSidebarLinks();
        bindScroll();
        bindHash();

        const search = document.getElementById("search");
        
        if (search) {
            search.addEventListener("input", function() {
                doSearch(this.value);
            });
        }

        if (location.hash) {
            activateFromHash();
        } else {
            requestSync(false);
        }
    }

    window.toggle = toggle;
    window.toggleFolder = toggleFolder;
    window.doSearch = doSearch;

    init();
})();