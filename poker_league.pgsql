-- PostgreSQL table schema for poker league 

-- try to modulize any rules that might change for each poker league season/game
-- 
create table plm_rulesets (
    rid           serial PRIMARY KEY,
    name          VARCHAR(256) -- short description of rules
);


CREATE TABLE plm_paysets (
    payid     serial PRIMARY KEY,
    name      VARCHAR(256), -- short description of rules
    buyin     INTEGER,
    minPot    INTEGER
);

-- individual payouts for paysets
CREATE TABLE plm_payouts (
    payid         INTEGER NOT NULL,
    numPlayers    INTEGER NOT NULL,
    place         INTEGER NOT NULL, 
    amount        INTEGER NOT NULL,
    PRIMARY KEY (payid,numPlayers,place),
    FOREIGN KEY (payid) REFERENCES plm_paysets (payid)
);

-- paysets associated with rulesets
CREATE TABLE plm_rulesetpaysets (
    rid INTEGER NOT NULL,
    payid INTEGER NOT NULL,
    FOREIGN KEY (rid) REFERENCES plm_rulesets (rid),
    FOREIGN KEY (payid) REFERENCES plm_paysets (payid),
    PRIMARY KEY (rid,payid)
);


CREATE TABLE plm_contribs (
    cid     SERIAL PRIMARY KEY,
    name    VARCHAR(32), 
    amount  INTEGER NOT NULL
);

CREATE TABLE plm_rulesetcontribs (
    rid INTEGER NOT NULL,
    cid INTEGER NOT NULL,
    FOREIGN KEY (rid) REFERENCES plm_rulesets (rid),
    FOREIGN KEY (cid) REFERENCES plm_contribs (cid),
    PRIMARY KEY (rid,cid)
);

-- records for each poker season, or league series or whatever
CREATE TABLE plm_seasons (
    sid      SERIAL PRIMARY KEY,
    ymdStart DATE,
    ymdEnd   DATE,
    name     VARCHAR(32)
);


CREATE TABLE plm_players (		
    pid   SERIAL PRIMARY KEY,	
    name  VARCHAR(32) NOT NULL UNIQUE
);	



-- record for each poker game
CREATE TABLE plm_games (
    gid 	SERIAL PRIMARY KEY, -- guid for this game
    sid   	INTEGER NOT NULL, -- seasonId
    rid         INTEGER NOT NULL, -- rulesetId
    ymd  	DATE NOT NULL,
    name 	VARCHAR(32), --eg 'Week 3'
    hostid      INTEGER, -- player id
    numPlayers 	INTEGER,   -- number of players in this game
    			   -- NULL means calculate values from 
                           -- #gid gamePlayers recs for this game
    
    FOREIGN KEY (rid)  REFERENCES plm_rulesets (rid) ,
    FOREIGN KEY (hostid) REFERENCES plm_players (pid) 
);

CREATE TABLE plm_gamePlayers (
    gid       INTEGER NOT NULL,   -- game id
    pid       INTEGER NOT NULL,   -- player id
    verified  BOOL DEFAULT FALSE, -- showed up, paid  
    PRIMARY KEY (gid,pid),
    FOREIGN KEY (pid) REFERENCES plm_players (pid),
    FOREIGN KEY (gid) REFERENCES plm_games (gid)
);

CREATE TABLE plm_gameResults (
    gid    INTEGER NOT NULL, -- game id
    pid    INTEGER NOT NULL, -- player id
    place  INTEGER NOT NULL, -- player place
    PRIMARY KEY (gid, pid, place),
    FOREIGN KEY (pid) REFERENCES plm_players (pid),
    FOREIGN KEY (gid) REFERENCES plm_games (gid)
);


INSERT INTO plm_players ( pid, name) 
VALUES 
(1, 'Partap'),
(2, 'Linda'),
(3, 'Abe of Spades'),
(4, 'Eliseo'),
(5, 'Kevin'),
(6, 'Baker'),
(7, 'Christi'),
(8, 'Bill'),
(9, 'Ilana'),
(10, 'Josh'),
(11, 'Jeremy'),
(12, 'Barber'),
(13, 'Webb'),
(14, 'TK'),
(15, 'Jack'),
(16, 'Blanchard'),
(17, 'Evans'),
(18, 'Chris H'),
(19, 'Joel'),
(20, 'Dennis'),
(21, 'Ryan'),
(22, 'Kelvin'),
(23, 'Judd'),
(24, 'Aran'),
(25, 'Anne'),
(26, 'Chochol');

-- some rules to start with...
-- we'll need a web interface for defining new rules and payouts later

INSERT INTO plm_rulesets ( rid, name) 
VALUES 
( 1, '2008 weekly game $10+2+2+1'),
( 2, '2008 final game');


INSERT INTO plm_seasons (sid,name,ymdStart,ymdEnd)
VALUES
(5,'Season 5 (2008 Jul-Dec)','2008-07-01','2008-12-31'), 
(6,'Season 6 (2009 Jan-Jun)','2009-01-01','2008-06-30');

INSERT INTO plm_games (gid,sid,rid,ymd,name,hostid,numPlayers) 
VALUES
(1,  5, 1, '2008-07-03', 'Week 1', 5, 28),
(2,  5, 1, '2008-07-10', 'Week 2', 15, 16),
(3,  5, 1, '2008-07-17', 'Week 3', 16, 27),
(27, 6, 1, '2009-01-01', 'Week 1', 3, 16),
(28, 6, 1, '2009-01-08', 'Week 2', 14, 21);

INSERT INTO plm_gameresults (gid, pid, place )
VALUES
(1, 14, 1),
(1, 7,  2),
(1, 10, 3),
(1, 8,  4),
(1, 17, 5),
(2, 18, 1),
(2, 19, 2),
(2, 10, 3),
(2, 20, 4),
(2, 21, 5),
(3, 22, 1),
(3, 17, 2),
(3, 15, 3),
(3, 16, 4),
(3, 23, 5),
(27, 24, 1),
(27, 3, 2),
(27, 25, 3),
(27, 5, 4),
(27, 23, 5),
(28, 11, 1),
(28, 12, 2),
(28, 13, 3),
(28, 6, 4),
(28, 14, 5);
